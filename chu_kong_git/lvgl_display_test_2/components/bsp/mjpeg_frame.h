#ifndef _MJPEG_FRAME_H_
#define _MJPEG_FRAME_H_
#include <stdint.h>

//jpeg数据单元
typedef struct
{
    uint8_t* frame;             //jpeg帧
    size_t len;                     //jpeg帧长
}jpeg_frame_data_t;

//mjpeg视频解析配置
typedef struct
{
    size_t  buff_size;          //分配的解析单元大小(根据实际情况，如果一帧图像数据大概有80KB,就写80*1024
}jpeg_frame_cfg_t;

/** 配置用于解析的数据
 * @param cfg 配置结构体
 * @return 无
*/
void jpeg_frame_config(jpeg_frame_cfg_t* cfg);

/** 启动jpeg图像解析
 * @param filename 文件名
 * @return 无
*/
void jpeg_frame_start(const char* filename);

/** 停止jpeg图像解析
 * @param 无
 * @return 无
*/
void jpeg_frame_stop(void);

/** 获取一个jpeg图像
 * @param data 返回的jpeg图像数据
 * @return 无
*/
void jpeg_frame_get_one(jpeg_frame_data_t* data);

#endif

