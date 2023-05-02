/*=======================================================================================
    Copyright (c) by Nanjing Tesiai Software Technology Co., Ltd.. All Rights Reserved.
 ========================================================================================*/

#include <stdint.h>

typedef enum {
  SCM_ADDR_HLOS_PHYSICAL = 0,
  SCM_ADDR_CRYPTO_VIRTUAL,
} SCM_ADDR_TYPE;

typedef enum {
  SCM_BUFF_NONSECURE = 0,
  SCM_BUFF_SECURE,
} SCM_BUFF_TYPE;

#pragma pack(push, 1)

/** Structure defined for handle passed from WFD when EArmHdcpBufferType is ARM_HDCP_BUFFER_TYPE_CRYPTO_VIRTUAL_POINTER
 */
typedef struct smmu_va_info {
    uint64_t  smmuVA;
    uint32_t  buffType;  //  SCM_BUFF_TYPE
} smmu_va_info_t, *psmmu_va_info_t;

#pragma pack(pop)
