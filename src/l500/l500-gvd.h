// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "gvd.h"

namespace librealsense
{
    namespace l500
    {
#pragma pack(1)
        typedef struct _rs_l500_gvd
        {
            number<uint16_t> StructureSize;
            number<uint8_t>   StructureVersion;
            number<uint8_t>   ProductType;
            number<uint8_t>   ProductID;
            number<uint8_t>   AdvancedModeEnabled;
            major_minor_version<uint8_t> AdvancedModeVersion;
            uint32_t padding1;
            change_set_version FunctionalPayloadVersion;
            major_minor_version<uint8_t> EyeSafetyPayloadVersion;
            uint16_t padding2;
            major_minor_version<uint8_t> DfuPayloadVersion;
            uint16_t padding3;
            number<uint8_t>   FlashRoVersion;
            number<uint8_t>   FlashStatus;
            number<uint8_t>   FlashRwVersion;
            uint8_t padding4;
            number<uint32_t> StrapState;
            number<uint32_t> OemId;
            number<uint32_t> oemVersion;
            number<uint16_t> MipiConfig;
            uint16_t padding5;
            number<uint32_t> MipiFrequencies;
            serial<6> OpticModuleSerial;
            uint16_t padding6;
            number<uint8_t>   OpticModuleVersion;
            uint8_t   padding7;
            uint8_t   padding8;
            uint8_t   padding9;
            number<uint32_t> OpticModuleMM;
            serial<6> AsicModuleSerial;
            uint16_t padding10;
            serial<6> AsicModuleChipID;
            uint16_t padding11;
            number<uint8_t>   AsicModuleVersion;
            uint8_t   padding12;
            uint8_t   padding13;
            uint8_t   padding14;
            number<uint32_t> AsicModuleMm;
            number<uint16_t> LeftSensorID;
            number<uint8_t>   LeftSensorVersion;
            uint32_t padding15;
            uint32_t padding16;
            uint8_t   padding17;
            number<uint16_t> RightSensorID;
            number<uint8_t>   RightSensorVersion;
            uint32_t padding18;
            uint32_t padding19;
            uint8_t   padding20;
            number<uint16_t> FishEyeSensorID;
            number<uint8_t>   FishEyeSensorVersion;
            uint32_t padding21;
            uint32_t padding22;
            uint8_t   padding23;
            number<uint8_t>   imuACCChipID;
            number<uint8_t>   imuGyroChipID;
            number<uint32_t> imuSTChipID;
            uint32_t padding24;
            uint16_t padding25;
            number<uint16_t> RgbModuleID;
            number<uint8_t>   RgbModuleVersion;
            uint8_t   padding26;
            serial<6> RgbModuleSN;
            uint16_t padding27;
            number<uint16_t> RgbIspFWVersion;
            uint32_t padding28;
            number<uint8_t>   WinUsbSupport;
            uint8_t   padding29;
            uint8_t   padding30;
            uint8_t   padding31;
            number<uint8_t>   HwType;
            uint8_t   padding32;
            uint8_t   padding33;
            uint8_t   padding34;
            number<uint8_t>   SkuComponent;
            uint8_t   padding35;
            uint8_t   padding36;
            uint8_t   padding37;
            number<uint8_t>   DepthCamera;
            uint8_t   padding38;
            uint8_t   padding39;
            uint8_t   padding40;
            number<uint8_t>   DepthActiveMode;
            uint8_t   padding41;
            uint8_t   padding42;
            uint8_t   padding43;
            number<uint8_t>   WithRGB;
            uint8_t   padding44;
            uint8_t   padding45;
            uint8_t   padding46;
            number<uint8_t>   WithImu;
            uint8_t   padding47;
            uint8_t   padding48;
            uint8_t   padding49;
            number<uint8_t>   ProjectorType;
            uint8_t   padding50;
            uint8_t   padding51;
            uint8_t   padding52;
            major_minor_version<uint8_t> EepromVersion;
            major_minor_version<uint8_t> EepromModuleInfoVersion;
            major_minor_version<uint8_t> AsicModuleInfoTableVersion;
            major_minor_version<uint8_t> CalibrationTableVersion;
            major_minor_version<uint8_t> CoefficientsVersion;
            major_minor_version<uint8_t> DepthVersion;
            major_minor_version<uint8_t> RgbVersion;
            major_minor_version<uint8_t> FishEyeVersion;
            major_minor_version<uint8_t> ImuVersion;
            major_minor_version<uint8_t> LensShadingVersion;
            major_minor_version<uint8_t> ProjectorVersion;
            major_minor_version<uint8_t> SNVersion;
            major_minor_version<uint8_t> ThermalLoopTableVersion;
            change_set_version MMFWVersion;
            serial<6> MMSN;
            uint16_t padding53;
            major_minor_version<uint16_t> MMVersion;
            number<uint8_t>   eepromLockStatus;
            uint8_t padding54;
        }rs_l500_gvd;
#pragma pack(pop)

    }
}
