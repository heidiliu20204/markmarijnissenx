/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_encoder.h"
#include "hdf_device_desc.h"
#include "osal_mem.h"
#include "osal_timer.h"
#include "gpio_if.h"
#include "hdf_log.h"
#include "event_hub.h"

#define TIMER_INTERVAL_ENCODER  10

static void EncoderTimerFunc(uintptr_t arg)
{
    EncoderDriver *encoderDrv = (EncoderDriver *)arg;
    uint16_t gpioClk = encoderDrv->encoderCfg->gpioClk;
    uint16_t gpioData = encoderDrv->encoderCfg->gpioData;
    uint16_t gpioSW = encoderDrv->encoderCfg->gpioSW;

    int32_t ret = GpioRead(gpioClk, &encoderDrv->encoderClkNowSta);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: gpio read failed, ret %d", __func__, ret);
        return;
    }
    ret = GpioRead(gpioData, &encoderDrv->encoderDataNowSta);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: gpio read failed, ret %d", __func__, ret);
        return;
    }

    if (encoderDrv->encoderClkNowSta != encoderDrv->encoderClkPreSta) {
        if (encoderDrv->encoderClkNowSta == 0) {
            if (encoderDrv->encoderDataNowSta == 1) {
                input_report_rel(encoderDrv->inputDev, REL_WHEEL, 1);
            } else {
                input_report_rel(encoderDrv->inputDev, REL_WHEEL, -1);
            }
            input_sync(encoderDrv->inputDev);
        }
        encoderDrv->encoderClkPreSta = encoderDrv->encoderClkNowSta;
        encoderDrv->encoderDataPreSta  = encoderDrv->encoderDataNowSta;
    }
}

static EncoderCfg *EncoderConfigInstance(struct HdfDeviceObject *device)
{
    EncoderCfg *encoderCfg = (EncoderCfg *)OsalMemAlloc(sizeof(EncoderCfg));
    if (encoderCfg == NULL) {
        HDF_LOGE("%s: malloc encoder config failed", __func__);
        return NULL;
    }
    int32_t ret = memset_s(encoderCfg, sizeof(EncoderCfg), 0, sizeof(EncoderCfg));
    if (ret != 0) {
        HDF_LOGE("%s: memset_s encoder config failed", __func__);
        OsalMemFree(encoderCfg);
        return NULL;
    }
    encoderCfg->hdfEncoderDev = device;

    if (ParseEncoderConfig(device->property, encoderCfg) != HDF_SUCCESS) {
        HDF_LOGE("%s: parse encoder config failed", __func__);
        OsalMemFree(encoderCfg);
        return NULL;
    }
    return encoderCfg;
}

static EncoderDriver *EncoderDriverInstance(EncoderCfg *encoderCfg)
{
    EncoderDriver *encoderDrv = (EncoderDriver *)OsalMemAlloc(sizeof(EncoderDriver));
    if (encoderDrv == NULL) {
        HDF_LOGE("%s: malloc key driver failed", __func__);
        return NULL;
    }
    int32_t ret = memset_s(encoderDrv, sizeof(EncoderDriver), 0, sizeof(EncoderDriver));
    if (ret != 0) {
        HDF_LOGE("%s: memset encoder driver failed", __func__);
        OsalMemFree(encoderDrv);
        return NULL;
    }

    encoderDrv->devType = encoderCfg->devType;
    encoderDrv->encoderCfg = encoderCfg;

    return encoderDrv;
}

static int32_t EncoderInit(EncoderDriver *EncoderDrv)
{
    uint16_t gpioClk = EncoderDrv->encoderCfg->gpioClk;
    uint16_t gpioData = EncoderDrv->encoderCfg->gpioData;
    uint16_t gpioSW = EncoderDrv->encoderCfg->gpioSW;
    GpioSetDir(gpioClk, GPIO_DIR_IN);
    GpioSetDir(gpioData, GPIO_DIR_IN);
    GpioSetDir(gpioSW, GPIO_DIR_IN);

    int32_t ret = OsalTimerCreate(&EncoderDrv->timer, TIMER_INTERVAL_ENCODER, EncoderTimerFunc, EncoderDrv);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: create timer failed, ret = %d\n", __func__, ret);
        return HDF_FAILURE;
    }
    ret = OsalTimerStartLoop(&EncoderDrv->timer);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: start timer failed, ret = %d\n", __func__, ret);
        return HDF_FAILURE;
    }

    ret = GpioRead(gpioClk, &EncoderDrv->encoderClkNowSta);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: gpio read failed, ret %d", __func__, ret);
        return HDF_FAILURE;
    }
    ret = GpioRead(gpioData, &EncoderDrv->encoderDataNowSta);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: gpio read failed, ret %d", __func__, ret);
        return HDF_FAILURE;
    }

    EncoderDrv->encoderClkPreSta = EncoderDrv->encoderClkNowSta;
    EncoderDrv->encoderDataPreSta = EncoderDrv->encoderDataNowSta;

    return HDF_SUCCESS;
}

static InputDevice *InputDeviceInstance(EncoderDriver *encoderDrv)
{
    InputDevice *inputDev = (InputDevice *)OsalMemAlloc(sizeof(InputDevice));
    if (inputDev == NULL) {
        HDF_LOGE("%s: malloc input device failed", __func__);
        return NULL;
    }
    int32_t ret = memset_s(inputDev, sizeof(InputDevice), 0, sizeof(InputDevice));
    if (ret != 0) {
        HDF_LOGE("%s: memset encoder driver failed", __func__);
        OsalMemFree(inputDev);
        return NULL;
    }

    inputDev->pvtData = (void *)encoderDrv;
    inputDev->devType = encoderDrv->devType;
    inputDev->hdfDevObj = encoderDrv->encoderCfg->hdfEncoderDev;
    encoderDrv->inputdev = inputDev;

    return inputDev;
}

static int32_t RegisterEncoderDevice(EncoderCfg *encoderCfg)
{
    EncoderDriver *EncoderDrv = EncoderDriverInstance(encoderCfg);
    if (EncoderDrv == NULL) {
        HDF_LOGE("%s: instance encoder driver failed", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    int32_t ret = EncoderInit(EncoderDrv);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: key driver init failed, ret %d", __func__, ret);
        goto EXIT;
    }

    InputDevice *inputDev = InputDeviceInstance(EncoderDrv);
    if (inputDev == NULL) {
        HDF_LOGE("%s: instance input device failed", __func__);
        goto EXIT;
    }

    ret = RegisterInputDevice(inputDev);
    if (ret != HDF_SUCCESS) {
        goto EXIT1;
    }
    return HDF_SUCCESS;

EXIT1:
    OsalMemFree(inputDev->pkgBuf);
    OsalMemFree(inputDev);
EXIT:
    OsalMemFree(EncoderDrv);
    return HDF_FAILURE;
}

static int32_t HdfEnCoderDriverInit(struct HdfDeviceObject *device)
{
    if (device == NULL) {
        HDF_LOGE("%s: param is null", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    EncoderCfg *encoderCfg = EncoderConfigInstance(device);
    if (encoderCfg == NULL) {
        HDF_LOGE("%s: instance encoder config failed", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    int32_t ret = RegisterEncoderDevice(encoderCfg);
    if (ret != HDF_SUCCESS) {
        goto EXIT;
    }

    HDF_LOGI("%s: exit succ!", __func__);
    return HDF_SUCCESS;

EXIT:
    OsalMemFree(encoderCfg);
    return HDF_FAILURE;
}

static int32_t HdfEnCoderDispatch(struct HdfDeviceIoClient *client, int cmd,
                                  struct HdfSBuf *data, struct HdfSBuf *reply)
{
    (void)cmd;
    if (client == NULL || data == NULL || reply == NULL) {
        HDF_LOGE("%s: param is null", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static void HdfEncoderDriverRelease(struct HdfDeviceObject *device)
{
    (void)device;
}

static int32_t HdfEnCoderDriverBind(struct HdfDeviceObject *device)
{
    if (device == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    static struct IDeviceIoService enCoderService = {
        .object.objectId = 1,
        .Dispatch = HdfEnCoderDispatch,
    };
    device->service = &enCoderService;
    return HDF_SUCCESS;
}

struct HdfDriverEntry g_hdfEnCoderEntry = {
    .moduleVersion = 1,
    .moduleName = "HDF_ENCODER",
    .Bind = HdfEnCoderDriverBind,
    .Init = HdfEnCoderDriverInit,
    .Release = HdfEncoderDriverRelease,
};

HDF_INIT(g_hdfEnCoderEntry);
