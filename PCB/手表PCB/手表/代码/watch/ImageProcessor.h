#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <Arduino.h>
#include <FS.h>         
#include <SdFat.h>
#include "PNGdec.h"
#include "JPEGDEC.h"

/**
 * @brief 图像处理器类，用于解码、缩放、裁剪PNG/JPEG格式图像
 * @details 支持PSRAM内存检查、进度回调、错误回调，
 *          可将任意尺寸的PNG/JPEG图像处理为指定尺寸240x280的RGB565格式图像
 */
class ImageProcessor {
public:
    /**
     * @brief 错误码枚举，标识图像处理过程中的各类错误
     */
    enum ErrorCode {
        SUCCESS = 0,                ///< 处理成功
        ERROR_FILE_NOT_FOUND,       ///< 文件未找到或读取失败
        ERROR_MEMORY_ALLOCATION,    ///< PSRAM内存分配失败（空间不足或分配失败）
        ERROR_IMAGE_DECODE,         ///< 图像解码失败（PNG/JPEG解析错误）
        ERROR_IMAGE_FORMAT,         ///< 不支持的图像格式（仅支持PNG/JPG/JPEG）
        ERROR_SCALE_FAILED,         ///< 图像缩放/裁剪/旋转失败
        ERROR_PROGRESSIVE_JPEG      ///< 不支持渐进式JPEG格式
    };

    /**
     * @brief 进度回调函数类型定义
     * @param percent 当前处理进度（0-100）
     */
    typedef void (*ProgressCallback)(int percent);

    /**
     * @brief 错误回调函数类型定义
     * @param error 错误码（ErrorCode枚举值）
     * @param message 错误描述信息
     */
    typedef void (*ErrorCallback)(ErrorCode error, const char* message);

    /**
     * @brief 构造函数，初始化图像处理器实例
     * @details 初始化所有成员变量为默认值，设置单例实例指针，默认目标尺寸为240x280
     */
    ImageProcessor();

    /**
     * @brief 析构函数，释放图像处理器占用的资源
     * @details 调用cleanup()释放所有已分配的图像内存，重置单例实例指针
     */
    ~ImageProcessor();

    /**
     * @brief 核心函数：加载并处理指定路径的图像文件
     * @details 完整处理流程：加载文件→解码PNG/JPEG→缩放/裁剪/旋转→输出目标尺寸图像
     *          支持横屏/竖屏自适应处理，自动释放中间过程内存，仅保留最终输出缓冲区
     * @param imagePath SD卡中的图像文件路径（如"/image.png"）
     * @param outputBuffer 输出缓冲区指针（指向处理后的RGB565格式图像数据，需外部手动释放）
     * @param outputSize 输出缓冲区总字节数（=目标宽度×目标高度×2，RGB565每个像素2字节）
     * @param targetWidth 目标图像宽度（默认240）
     * @param targetHeight 目标图像高度（默认280）
     * @return ErrorCode 处理结果，SUCCESS表示成功，其他值为对应错误码
     * @note 1. 仅支持PNG、JPG/JPEG格式，不支持渐进式JPEG；
     *       2. 输出缓冲区由该函数分配（PSRAM），调用者需负责最终释放（使用free()）；
     *       3. 处理过程中会自动清理之前的图像内存，无需手动调用cleanup()。
     */
    ErrorCode loadAndProcessImage(const char* imagePath, 
                                  uint16_t** outputBuffer, 
                                  size_t* outputSize,
                                  int targetWidth = 240,
                                  int targetHeight = 280);

    /**
     * @brief 设置进度回调函数
     * @details 处理过程中（解码、缩放）会按进度百分比触发该回调，若未设置则默认打印到串口
     * @param callback 进度回调函数指针（可为nullptr，取消回调）
     */
    void setProgressCallback(ProgressCallback callback);

    /**
     * @brief 设置错误回调函数
     * @details 处理过程中发生错误时触发该回调，若未设置则默认打印错误信息到串口
     * @param callback 错误回调函数指针（可为nullptr，取消回调）
     */
    void setErrorCallback(ErrorCallback callback);

    /**
     * @brief 获取最后一次处理的图像原始信息
     * @details 需在loadAndProcessImage调用成功后使用，参数可为nullptr（忽略对应字段）
     * @param format 输出参数，图像格式字符串（"PNG"/"JPEG"/"N/A"）
     * @param originalWidth 输出参数，图像原始宽度
     * @param originalHeight 输出参数，图像原始高度
     */
    void getImageInfo(const char** format, int* originalWidth, int* originalHeight);

    /**
     * @brief 获取当前可用的PSRAM（伪静态RAM）大小
     * @details 用于外部检查内存状态，单位为字节（Byte）
     * @return size_t 可用PSRAM字节数
     */
    size_t getFreePSRAM();

    /**
     * @brief 检查PSRAM是否满足指定内存需求
     * @details 会额外预留1KB安全空间，避免内存耗尽；若不足会打印内存信息到串口
     * @param requiredSize 需要的内存字节数
     * @param operation 操作名称（用于错误日志，如"原始PNG图像"）
     * @return true 内存足够（包含1KB安全空间），false 内存不足
     */
    bool checkPSRAM(size_t requiredSize, const char* operation);

private:
    // 成员变量
    uint16_t* rawImage;          // 原始图像数据（JPEG/PNG解码后，RGB565格式）
    uint16_t* scaledImage;       // 缩放后的图像数据（RGB565格式）
    uint32_t rawWidth;           // 原始图像宽度
    uint32_t rawHeight;          // 原始图像高度
    uint32_t scaledWidth;        // 缩放后图像宽度
    uint32_t scaledHeight;       // 缩放后图像高度
    
    PNG png;                     // PNG解码实例
    JPEGDEC jpeg;                // JPEG解码实例
    
    ProgressCallback progressCallback; // 进度回调函数指针
    ErrorCallback errorCallback;       // 错误回调函数指针
    
    char lastImageFormat[8];     // 最后处理的图像格式（"PNG"/"JPEG"/"N/A"）
    int lastOriginalWidth;       // 最后处理图像的原始宽度
    int lastOriginalHeight;      // 最后处理图像的原始高度
    
    int targetWidth;             // 目标图像宽度
    int targetHeight;            // 目标图像高度
    
    // 单例实例指针
    static ImageProcessor* instance;
    
    // 私有方法
    ErrorCode loadFileToBuffer(const char* imagePath, uint8_t** buffer, size_t* fileSize);
    ErrorCode processPNG(uint8_t* fileBuffer, size_t fileSize);
    ErrorCode processJPEG(uint8_t* fileBuffer, size_t fileSize);
    bool scaleAndCropImage(uint16_t* src, int srcWidth, int srcHeight);
    uint16_t* rotateImage90(uint16_t* src, int srcWidth, int srcHeight);
    void cleanup();
    
    // 静态回调函数
    static void staticPngDrawCallback(PNGDRAW *pDraw);
    static int staticJpegDecodeCallback(JPEGDRAW *pDraw);
    
    // 成员回调函数
    void pngDrawCallback(PNGDRAW *pDraw);
    int jpegDecodeCallback(JPEGDRAW *pDraw);
    
    // 内部报告函数
    void reportProgress(int percent);
    void reportError(ErrorCode error, const char* message);
};

#endif