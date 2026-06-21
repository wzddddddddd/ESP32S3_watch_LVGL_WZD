#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "mjpeg_frame.h"
#include "esp_log.h"

#define TAG     "mjpeg_frame"

#define JPEG_SOI 0xFFD8     //JPEG帧起始标志
#define JPEG_EOI 0xFFD9     //JPEG帧结束标志

#define MIN_READ_BUFF_SIZE      4096

#define BUFF_SIZE      1024*80

#define START_GET_EV    (BIT0)
#define STOP_GET_EV     (BIT1)
#define DONE_GET_EV     (BIT2)  // 任务已完全退出

static EventGroupHandle_t   mjpeg_event;
static QueueHandle_t        mjpeg_data_queue;

static jpeg_frame_cfg_t  s_mjpeg_cfg;
static int mjpeg_inited = 0;        //初始化标志
static int mjpeg_started = 0;       //已经启动解析标志

/** 配置用于解析的数据
 * @param cfg 配置结构体
 * @return 无
*/
void jpeg_frame_config(jpeg_frame_cfg_t* cfg)
{
    if (cfg->buff_size < MIN_READ_BUFF_SIZE)
        return;
    memcpy(&s_mjpeg_cfg,cfg,sizeof(jpeg_frame_cfg_t));
    if(!mjpeg_event)
        mjpeg_event = xEventGroupCreate();
    if(!mjpeg_data_queue)
        mjpeg_data_queue = xQueueCreate(5,sizeof(jpeg_frame_data_t));
    mjpeg_inited = 1;
}

static void jpeg_export_frame_task(void* param);

/** 启动jpeg图像解析
 * @param filename 文件名
 * @return 无
*/
void jpeg_frame_start(const char* filename)
{
    if (!mjpeg_inited)
        return;
    if(mjpeg_started)
        return;
    // 清除残留的停止标志，避免新任务被旧状态卡住
    if (mjpeg_event != NULL) {
        xEventGroupClearBits(mjpeg_event, STOP_GET_EV | START_GET_EV);
    }
    // filepath 已是完整路径 /sdcard/xxx，直接使用
    mjpeg_started = 1;
    xTaskCreatePinnedToCore(jpeg_export_frame_task,"mjpeg",4096,(void*)filename,6,NULL,1);

}

/** mjpeg视频解析任务
 * @param param 文件名
 * @return 无
*/
static void jpeg_export_frame_task(void* param)
{
    char *filename = (char*)param;
    FILE* f = fopen(filename, "rb");
    int frame_cnt = 0;      //用于统计解析出来的帧数
    int jpeg_started = 0;
    uint8_t* frame_buff = NULL;     //截取到的JPG帧（移到开头，避免goto时未初始化）
    size_t frame_write_index = 0;   //当前的JPG帧的长度
    size_t frame_buff_total_len = s_mjpeg_cfg.buff_size;    //分配给JPG帧的内存大小
    if (f == NULL)
    {
        printf("Can't open file:%s\r\n",filename);
        goto mjpeg_task_del;
    }
    uint8_t* read_buff = (uint8_t*)malloc(s_mjpeg_cfg.buff_size);
    if (!read_buff)
    {
        goto mjpeg_task_del;
    }
    size_t read_bytes = 0;          //每次实际读取文件的字节数
    while ((read_bytes = fread(read_buff, 1, s_mjpeg_cfg.buff_size, f) )> 0)
    {
        int soi_index = 0;      //SOI的下标
        for (int i = 0; i < read_bytes - 1; i++)
        {
            uint16_t oi_flags = (read_buff[i] << 8 )+ read_buff[i+1];   //查找标志
            if (!jpeg_started && oi_flags == JPEG_SOI)      //读回来的数据中检索到JPEG起始标志
            {
                soi_index = i;
                jpeg_started = 1;
            }
            else if(jpeg_started && oi_flags == JPEG_EOI)   //读回来的数据中检索到JPEG结束标志
            {
                int write_len = (i - soi_index + 1) + 1;    //+1表示把JPEG_EOI也进行拷贝
                if (frame_buff)
                {
                    if (frame_write_index + write_len <= frame_buff_total_len)  //查看当前分配的jpeg帧内存释放足够存放
                    {
                        memcpy(&frame_buff[frame_write_index], &read_buff[soi_index], write_len);
                    }
                    else    //不足够存放，需要重新分配内存单元，进行写入
                    {
                        size_t new_size = frame_write_index + write_len;
                        uint8_t *new_buf = (uint8_t*)realloc(frame_buff, new_size);
                        if (new_buf == NULL) {
                            free(frame_buff);
                            frame_buff = NULL;
                            goto mjpeg_task_del;
                        }
                        frame_buff = new_buf;
                        frame_buff_total_len = new_size;
                        memcpy(&frame_buff[frame_write_index], &read_buff[soi_index], write_len);
                    }
                }
                else
                {
                    frame_buff = (uint8_t*)malloc(write_len);
                    if (frame_buff == NULL) {
                        goto mjpeg_task_del;
                    }
                    memcpy(&frame_buff[frame_write_index], &read_buff[soi_index], write_len);
                }
                jpeg_frame_data_t frame_data;
                frame_data.frame = frame_buff;
                frame_data.len = frame_write_index + write_len;
                frame_cnt++;
                //ESP_LOGI(TAG,"jpg frame detect.NO.:%i,len:%d",frame_cnt,frame_data.len);
                //这里已经提取到了一帧jpeg图像，等到读取事件，然后把数据放入队列
                EventBits_t ev = xEventGroupWaitBits(mjpeg_event,START_GET_EV|STOP_GET_EV,pdTRUE,pdFALSE,portMAX_DELAY);
                if(ev & START_GET_EV)
                {
                    // 每100ms检测一次停止标志，防止死锁
                    while (xQueueSend(mjpeg_data_queue, &frame_data, pdMS_TO_TICKS(100)) != pdTRUE) {
                        if (xEventGroupGetBits(mjpeg_event) & STOP_GET_EV) {
                            if (frame_data.frame) free(frame_data.frame);
                            goto mjpeg_task_del;
                        }
                    }
                }
                if(ev & STOP_GET_EV)
                {
                    if(frame_buff)
                        free(frame_buff);
                    goto mjpeg_task_del;
                }
                
                frame_buff = NULL;      //frame_buff已经通过队列发送出去了，这里置NULL，下次重新查找到SOI和SOE后将重新malloc
                frame_write_index = 0;
                jpeg_started = 0;
            }
        }
        //上面的一个检索循环结束后，检测到了起始标志，但没有检测到结束标志，需要把数据拷贝到frame_buff里
        //待下次的检索循环检索到后，结束标志后，就可以追加后面的内容，确保不漏掉一帧
        if (jpeg_started)
        {
            size_t write_len = s_mjpeg_cfg.buff_size - soi_index;   //长度为当前从文件读回来的数据减去SOI下标
            if (frame_buff)
            {
                if (frame_write_index + write_len > frame_buff_total_len)
                {
                    size_t new_size = frame_write_index + write_len;
                    uint8_t *new_buf = (uint8_t*)realloc(frame_buff, new_size);
                    if (new_buf == NULL) {
                        free(frame_buff);
                        frame_buff = NULL;
                        goto mjpeg_task_del;
                    }
                    frame_buff = new_buf;
                    frame_buff_total_len = new_size;
                }
                memcpy(&frame_buff[frame_write_index], &read_buff[soi_index], write_len);
                frame_write_index += write_len;
            }
            else
            {
                if (write_len > frame_buff_total_len)
                    frame_buff_total_len = write_len;
                frame_buff = (uint8_t*)malloc(frame_buff_total_len);
                if (frame_buff == NULL) {
                    goto mjpeg_task_del;
                }
                memcpy(&frame_buff[0], &read_buff[soi_index], write_len);
                frame_write_index = write_len;
            }
        }
    }
mjpeg_task_del:
    // 通知 jpeg_frame_stop()：任务已退出
    if (mjpeg_event) {
        xEventGroupSetBits(mjpeg_event, DONE_GET_EV);
    }
    // 清空队列残留（不 free，因为 esp_jpg_decode 可能已破坏堆）
    jpeg_frame_data_t tmp;
    while (xQueueReceive(mjpeg_data_queue, &tmp, 0) == pdTRUE) {
        // 不 free(tmp.frame)：解码失败时堆已破坏，free 会崩溃
    }
    // 不 free(frame_buff)：esp_jpg_decode 解码失败时可能已破坏堆，继续 free 会崩溃
    frame_buff = NULL;
    if(f)
        fclose(f);
    mjpeg_started = 0;
    // 销毁事件组和队列，下次启动时重新创建
    if (mjpeg_event) {
        vEventGroupDelete(mjpeg_event);
        mjpeg_event = NULL;
    }
    if (mjpeg_data_queue) {
        vQueueDelete(mjpeg_data_queue);
        mjpeg_data_queue = NULL;
    }
    vTaskDelete(NULL);
}

/** 获取一个jpeg图像
 * @param data 返回的jpeg图像数据
 * @return 无
*/
void jpeg_frame_get_one(jpeg_frame_data_t* data)
{
    jpeg_frame_data_t frame_data = {0, 0};
    if (!mjpeg_started || mjpeg_event == NULL)
        return;
    xEventGroupSetBits(mjpeg_event, START_GET_EV);   //标记一个获取事件
    if (pdTRUE == xQueueReceive(mjpeg_data_queue, &frame_data, pdMS_TO_TICKS(0)))   //不阻塞，由video_task的帧率控制决定节奏
    {
        memcpy(data, &frame_data, sizeof(jpeg_frame_data_t));
        return;
    }
    else
    {
        data->len = 0;  //这里简单判断，返回0就认为播放结束
    }
}

/** 停止jpeg图像解析
 * @param 无
 * @return 无
*/
void jpeg_frame_stop(void)
{
    if (mjpeg_event != NULL) {
        xEventGroupSetBits(mjpeg_event, STOP_GET_EV);  // 标记一个结束事件
        // 等待 mjpeg_export_frame_task 完全退出（DONE_GET_EV）后再返回
        xEventGroupWaitBits(mjpeg_event, DONE_GET_EV, pdTRUE, pdFALSE, pdMS_TO_TICKS(500));
    }
}
