#include "onenet_dm.h"
#include <string.h>

/**
 * 物模型数据初始化
 * @param 无
 * @return 无
 */
void onenet_dm_init(void)
{
    // 空实现，后续按需扩展
}

/**
 * 处理onenet下行的数据
 * @param property_js 包含下行数据的json
 * @return 无
 */
void onenet_property_handle(cJSON* property_js)
{
    // 暂不处理任何属性，后续按需扩展
    (void)property_js;
}

/**
 * 生成上报所有数据的cJSON对象
 * @param 无
 * @return cJSON对象，包含所有属性值
 */
cJSON* onenet_property_upload_dm(void)
{
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "id", "123");
    cJSON_AddStringToObject(root, "version", "1.0");
    cJSON* params_js = cJSON_AddObjectToObject(root, "params");
    (void)params_js;
    return root;
}
