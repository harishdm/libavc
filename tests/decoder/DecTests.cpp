/******************************************************************************
 *
 * Copyright (C) 2026 Ittiam Systems Pvt Ltd, Bangalore
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

#include <gtest/gtest.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "DecHelper.h"
#include "TestEnums.h"

namespace libavc {
namespace test {

struct DecodeStreamConfig {
  std::string inputFile;
  Format format;
  std::string refMd5File;
};

void PrintTo(const DecodeStreamConfig& config, ::std::ostream* os) {
  *os << config.inputFile;
}

// Helper to resolve absolute file path for test assets
static std::string getFullPath(const std::string& inputFile) {
  if (const char* envPath = std::getenv("AVC_TEST_DIR")) {
    return std::string(envPath) + "/" + inputFile;
  }
  return inputFile;
}

class DecTestFixture
    : public ::testing::TestWithParam<std::tuple<DecodeStreamConfig, size_t>> {
 protected:
  DecTestFixture() = default;
  ~DecTestFixture() override = default;
};

TEST_P(DecTestFixture, DecodeVerify) {
  auto [config, cores] = GetParam();

  std::string inputPath = getFullPath(config.inputFile);
  std::string refMd5Path = getFullPath(config.refMd5File);

  auto builder = DecHelper::Builder()
                     .setInputFilePath(inputPath)
                     .setRefMd5Path(refMd5Path)
                     .setFormat(config.format)
                     .setCores(cores);

  std::unique_ptr<DecHelper> helper = builder.build();
  ASSERT_NE(helper, nullptr)
      << "Failed to build DecHelper for input: " << inputPath;
  EXPECT_TRUE(helper->decodeFile())
      << "Decoding failed for input: " << inputPath;
}

static const std::vector<DecodeStreamConfig> kDecodeStreams = {
    {"AUD_MW_E/AUD_MW_E.264", Format::yuv420p, "AUD_MW_E/AUD_MW_E_md5.txt"},
    {"BA1_FT_C/BA1_FT_C.264", Format::yuv420p, "BA1_FT_C/BA1_FT_C_md5.txt"},
    {"BA1_Sony_D/BA1_Sony_D.jsv", Format::yuv420p, "BA1_Sony_D/BA1_Sony_D_md5.txt"},
    {"BA2_Sony_F/BA2_Sony_F.jsv", Format::yuv420p, "BA2_Sony_F/BA2_Sony_F_md5.txt"},
    {"BA3_SVA_C/BA3_SVA_C.264", Format::yuv420p, "BA3_SVA_C/BA3_SVA_C_md5.txt"},
    {"BAMQ1_JVC_C/BAMQ1_JVC_C.264", Format::yuv420p, "BAMQ1_JVC_C/BAMQ1_JVC_C_md5.txt"},
    {"BAMQ2_JVC_C/BAMQ2_JVC_C.264", Format::yuv420p, "BAMQ2_JVC_C/BAMQ2_JVC_C_md5.txt"},
    {"BANM_MW_D/BANM_MW_D.264", Format::yuv420p, "BANM_MW_D/BANM_MW_D_md5.txt"},
    {"BASQP1_Sony_C/BASQP1_Sony_C.jsv", Format::yuv420p, "BASQP1_Sony_C/BASQP1_Sony_C_md5.txt"},
    {"BA_MW_D/BA_MW_D.264", Format::yuv420p, "BA_MW_D/BA_MW_D_md5.txt"},
    {"CABA1_SVA_B/CABA1_SVA_B.264", Format::yuv420p, "CABA1_SVA_B/CABA1_SVA_B_md5.txt"},
    {"CABA1_Sony_D/CABA1_Sony_D.jsv", Format::yuv420p, "CABA1_Sony_D/CABA1_Sony_D_md5.txt"},
    {"CABA2_SVA_B/CABA2_SVA_B.264", Format::yuv420p, "CABA2_SVA_B/CABA2_SVA_B_md5.txt"},
    {"CABA2_Sony_E/CABA2_Sony_E.jsv", Format::yuv420p, "CABA2_Sony_E/CABA2_Sony_E_md5.txt"},
    {"CABA3_SVA_B/CABA3_SVA_B.264", Format::yuv420p, "CABA3_SVA_B/CABA3_SVA_B_md5.txt"},
    {"CABA3_Sony_C/CABA3_Sony_C.jsv", Format::yuv420p, "CABA3_Sony_C/CABA3_Sony_C_md5.txt"},
    {"CABA3_TOSHIBA_E/CABA3_TOSHIBA_E.264", Format::yuv420p, "CABA3_TOSHIBA_E/CABA3_TOSHIBA_E_md5.txt"},
    {"CABACI3_Sony_B/CABACI3_Sony_B.jsv", Format::yuv420p, "CABACI3_Sony_B/CABACI3_Sony_B_md5.txt"},
    {"CABAST3_Sony_E/CABAST3_Sony_E.jsv", Format::yuv420p, "CABAST3_Sony_E/CABAST3_Sony_E_md5.txt"},
    {"CABASTBR3_Sony_B/CABASTBR3_Sony_B.jsv", Format::yuv420p, "CABASTBR3_Sony_B/CABASTBR3_Sony_B_md5.txt"},
    {"CABREF3_Sand_D/CABREF3_Sand_D.264", Format::yuv420p, "CABREF3_Sand_D/CABREF3_Sand_D_md5.txt"},
    {"CACQP3_Sony_D/CACQP3_Sony_D.jsv", Format::yuv420p, "CACQP3_Sony_D/CACQP3_Sony_D_md5.txt"},
    {"CAFI1_SVA_C/CAFI1_SVA_C.264", Format::yuv420p, "CAFI1_SVA_C/CAFI1_SVA_C_md5.txt"},
    {"CAMA1_Sony_C/CAMA1_Sony_C.jsv", Format::yuv420p, "CAMA1_Sony_C/CAMA1_Sony_C_md5.txt"},
    {"CAMA1_TOSHIBA_B/CAMA1_TOSHIBA_B.264", Format::yuv420p, "CAMA1_TOSHIBA_B/CAMA1_TOSHIBA_B_md5.txt"},
    {"CAMA3_Sand_E/CAMA3_Sand_E.264", Format::yuv420p, "CAMA3_Sand_E/CAMA3_Sand_E_md5.txt"},
    {"CAMACI3_Sony_C/CAMACI3_Sony_C.jsv", Format::yuv420p, "CAMACI3_Sony_C/CAMACI3_Sony_C_md5.txt"},
    {"CAMANL1_TOSHIBA_B/CAMANL1_TOSHIBA_B.264", Format::yuv420p, "CAMANL1_TOSHIBA_B/CAMANL1_TOSHIBA_B_md5.txt"},
    {"CAMANL2_TOSHIBA_B/CAMANL2_TOSHIBA_B.264", Format::yuv420p, "CAMANL2_TOSHIBA_B/CAMANL2_TOSHIBA_B_md5.txt"},
    {"CAMANL3_Sand_E/CAMANL3_Sand_E.264", Format::yuv420p, "CAMANL3_Sand_E/CAMANL3_Sand_E_md5.txt"},
    {"CAMASL3_Sony_B/CAMASL3_Sony_B.jsv", Format::yuv420p, "CAMASL3_Sony_B/CAMASL3_Sony_B_md5.txt"},
    {"CAMP_MOT_MBAFF_L30/CAMP_MOT_MBAFF_L30.26l", Format::yuv420p, "CAMP_MOT_MBAFF_L30/CAMP_MOT_MBAFF_L30_md5.txt"},
    {"CAMP_MOT_MBAFF_L31/CAMP_MOT_MBAFF_L31.26l", Format::yuv420p, "CAMP_MOT_MBAFF_L31/CAMP_MOT_MBAFF_L31_md5.txt"},
    {"CANL1_SVA_B/CANL1_SVA_B.264", Format::yuv420p, "CANL1_SVA_B/CANL1_SVA_B_md5.txt"},
    {"CANL1_Sony_E/CANL1_Sony_E.jsv", Format::yuv420p, "CANL1_Sony_E/CANL1_Sony_E_md5.txt"},
    {"CANL1_TOSHIBA_G/CANL1_TOSHIBA_G.264", Format::yuv420p, "CANL1_TOSHIBA_G/CANL1_TOSHIBA_G_md5.txt"},
    {"CANL2_SVA_B/CANL2_SVA_B.264", Format::yuv420p, "CANL2_SVA_B/CANL2_SVA_B_md5.txt"},
    {"CANL2_Sony_E/CANL2_Sony_E.jsv", Format::yuv420p, "CANL2_Sony_E/CANL2_Sony_E_md5.txt"},
    {"CANL3_SVA_B/CANL3_SVA_B.264", Format::yuv420p, "CANL3_SVA_B/CANL3_SVA_B_md5.txt"},
    {"CANL3_Sony_C/CANL3_Sony_C.jsv", Format::yuv420p, "CANL3_Sony_C/CANL3_Sony_C_md5.txt"},
    {"CANL4_SVA_B/CANL4_SVA_B.264", Format::yuv420p, "CANL4_SVA_B/CANL4_SVA_B_md5.txt"},
    {"CANLMA2_Sony_C/CANLMA2_Sony_C.jsv", Format::yuv420p, "CANLMA2_Sony_C/CANLMA2_Sony_C_md5.txt"},
    {"CANLMA3_Sony_C/CANLMA3_Sony_C.jsv", Format::yuv420p, "CANLMA3_Sony_C/CANLMA3_Sony_C_md5.txt"},
    {"CAPA1_TOSHIBA_B/CAPA1_TOSHIBA_B.264", Format::yuv420p, "CAPA1_TOSHIBA_B/CAPA1_TOSHIBA_B_md5.txt"},
    {"CAPAMA3_Sand_F/CAPAMA3_Sand_F.264", Format::yuv420p, "CAPAMA3_Sand_F/CAPAMA3_Sand_F_md5.txt"},
    {"CAPCM1_Sand_E/CAPCM1_Sand_E.264", Format::yuv420p, "CAPCM1_Sand_E/CAPCM1_Sand_E_md5.txt"},
    {"CAPCMNL1_Sand_E/CAPCMNL1_Sand_E.264", Format::yuv420p, "CAPCMNL1_Sand_E/CAPCMNL1_Sand_E_md5.txt"},
    {"CAPM3_Sony_D/CAPM3_Sony_D.jsv", Format::yuv420p, "CAPM3_Sony_D/CAPM3_Sony_D_md5.txt"},
    {"CAQP1_Sony_B/CAQP1_Sony_B.jsv", Format::yuv420p, "CAQP1_Sony_B/CAQP1_Sony_B_md5.txt"},
    {"CAWP1_TOSHIBA_E/CAWP1_TOSHIBA_E.264", Format::yuv420p, "CAWP1_TOSHIBA_E/CAWP1_TOSHIBA_E_md5.txt"},
    {"CAWP5_TOSHIBA_E/CAWP5_TOSHIBA_E.264", Format::yuv420p, "CAWP5_TOSHIBA_E/CAWP5_TOSHIBA_E_md5.txt"},
    {"CI1_FT_B/CI1_FT_B.264", Format::yuv420p, "CI1_FT_B/CI1_FT_B_md5.txt"},
    {"CI_MW_D/CI_MW_D.264", Format::yuv420p, "CI_MW_D/CI_MW_D_md5.txt"},
    {"CVBS3_Sony_C/CVBS3_Sony_C.jsv", Format::yuv420p, "CVBS3_Sony_C/CVBS3_Sony_C_md5.txt"},
    {"CVCANLMA2_Sony_C/CVCANLMA2_Sony_C.jsv", Format::yuv420p, "CVCANLMA2_Sony_C/CVCANLMA2_Sony_C_md5.txt"},
    {"CVFC1_Sony_C/CVFC1_Sony_C.jsv", Format::yuv420p, "CVFC1_Sony_C/CVFC1_Sony_C_md5.txt"},
    {"CVFI1_SVA_C/CVFI1_SVA_C.264", Format::yuv420p, "CVFI1_SVA_C/CVFI1_SVA_C_md5.txt"},
    {"CVFI1_Sony_D/CVFI1_Sony_D.jsv", Format::yuv420p, "CVFI1_Sony_D/CVFI1_Sony_D_md5.txt"},
    {"CVFI2_SVA_C/CVFI2_SVA_C.264", Format::yuv420p, "CVFI2_SVA_C/CVFI2_SVA_C_md5.txt"},
    {"CVFI2_Sony_H/CVFI2_Sony_H.jsv", Format::yuv420p, "CVFI2_Sony_H/CVFI2_Sony_H_md5.txt"},
    {"CVMA1_Sony_D/CVMA1_Sony_D.jsv", Format::yuv420p, "CVMA1_Sony_D/CVMA1_Sony_D_md5.txt"},
    {"CVMA1_TOSHIBA_B/CVMA1_TOSHIBA_B.264", Format::yuv420p, "CVMA1_TOSHIBA_B/CVMA1_TOSHIBA_B_md5.txt"},
    {"CVMANL1_TOSHIBA_B/CVMANL1_TOSHIBA_B.264", Format::yuv420p, "CVMANL1_TOSHIBA_B/CVMANL1_TOSHIBA_B_md5.txt"},
    {"CVMANL2_TOSHIBA_B/CVMANL2_TOSHIBA_B.264", Format::yuv420p, "CVMANL2_TOSHIBA_B/CVMANL2_TOSHIBA_B_md5.txt"},
    {"CVMAPAQP3_Sony_E/CVMAPAQP3_Sony_E.jsv", Format::yuv420p, "CVMAPAQP3_Sony_E/CVMAPAQP3_Sony_E_md5.txt"},
    {"CVMAQP2_Sony_G/CVMAQP2_Sony_G.jsv", Format::yuv420p, "CVMAQP2_Sony_G/CVMAQP2_Sony_G_md5.txt"},
    {"CVMAQP3_Sony_D/CVMAQP3_Sony_D.jsv", Format::yuv420p, "CVMAQP3_Sony_D/CVMAQP3_Sony_D_md5.txt"},
    {"CVMP_MOT_FLD_L30_B/CVMP_MOT_FLD_L30_B.26l", Format::yuv420p, "CVMP_MOT_FLD_L30_B/CVMP_MOT_FLD_L30_B_md5.txt"},
    {"CVMP_MOT_FRM_L31_B/CVMP_MOT_FRM_L31_B.26l", Format::yuv420p, "CVMP_MOT_FRM_L31_B/CVMP_MOT_FRM_L31_B_md5.txt"},
    {"CVNLFI1_Sony_C/CVNLFI1_Sony_C.jsv", Format::yuv420p, "CVNLFI1_Sony_C/CVNLFI1_Sony_C_md5.txt"},
    {"CVNLFI2_Sony_H/CVNLFI2_Sony_H.jsv", Format::yuv420p, "CVNLFI2_Sony_H/CVNLFI2_Sony_H_md5.txt"},
    {"CVPA1_TOSHIBA_B/CVPA1_TOSHIBA_B.264", Format::yuv420p, "CVPA1_TOSHIBA_B/CVPA1_TOSHIBA_B_md5.txt"},
    {"CVPCMNL1_SVA_C/CVPCMNL1_SVA_C.264", Format::yuv420p, "CVPCMNL1_SVA_C/CVPCMNL1_SVA_C_md5.txt"},
    {"CVPCMNL2_SVA_C/CVPCMNL2_SVA_C.264", Format::yuv420p, "CVPCMNL2_SVA_C/CVPCMNL2_SVA_C_md5.txt"},
    {"CVSE2_Sony_B/CVSE2_Sony_B.jsv", Format::yuv420p, "CVSE2_Sony_B/CVSE2_Sony_B_md5.txt"},
    {"CVSE3_Sony_H/CVSE3_Sony_H.jsv", Format::yuv420p, "CVSE3_Sony_H/CVSE3_Sony_H_md5.txt"},
    {"CVSEFDFT3_Sony_E/CVSEFDFT3_Sony_E.jsv", Format::yuv420p, "CVSEFDFT3_Sony_E/CVSEFDFT3_Sony_E_md5.txt"},
    {"CVWP1_TOSHIBA_E/CVWP1_TOSHIBA_E.264", Format::yuv420p, "CVWP1_TOSHIBA_E/CVWP1_TOSHIBA_E_md5.txt"},
    {"CVWP2_TOSHIBA_E/CVWP2_TOSHIBA_E.264", Format::yuv420p, "CVWP2_TOSHIBA_E/CVWP2_TOSHIBA_E_md5.txt"},
    {"CVWP3_TOSHIBA_E/CVWP3_TOSHIBA_E.264", Format::yuv420p, "CVWP3_TOSHIBA_E/CVWP3_TOSHIBA_E_md5.txt"},
    {"CVWP5_TOSHIBA_E/CVWP5_TOSHIBA_E.264", Format::yuv420p, "CVWP5_TOSHIBA_E/CVWP5_TOSHIBA_E_md5.txt"},
    {"FI1_Sony_E/FI1_Sony_E.jsv", Format::yuv420p, "FI1_Sony_E/FI1_Sony_E_md5.txt"},
    {"HCBP1_HHI_A/HCBP1_HHI_A.264", Format::yuv420p, "HCBP1_HHI_A/HCBP1_HHI_A_md5.txt"},
    {"HCBP2_HHI_A/HCBP2_HHI_A.264", Format::yuv420p, "HCBP2_HHI_A/HCBP2_HHI_A_md5.txt"},
    {"HCMP1_HHI_A/HCMP1_HHI_A.264", Format::yuv420p, "HCMP1_HHI_A/HCMP1_HHI_A_md5.txt"},
    {"LS_SVA_D/LS_SVA_D.264", Format::yuv420p, "LS_SVA_D/LS_SVA_D_md5.txt"},
    {"MIDR_MW_D/MIDR_MW_D.264", Format::yuv420p, "MIDR_MW_D/MIDR_MW_D_md5.txt"},
    {"MPS_MW_A/MPS_MW_A.264", Format::yuv420p, "MPS_MW_A/MPS_MW_A_md5.txt"},
    {"MR1_BT_A/MR1_BT_A.h264", Format::yuv420p, "MR1_BT_A/MR1_BT_A_md5.txt"},
    {"MR1_MW_A/MR1_MW_A.264", Format::yuv420p, "MR1_MW_A/MR1_MW_A_md5.txt"},
    {"MR2_MW_A/MR2_MW_A.264", Format::yuv420p, "MR2_MW_A/MR2_MW_A_md5.txt"},
    {"MR2_TANDBERG_E/MR2_TANDBERG_E.264", Format::yuv420p, "MR2_TANDBERG_E/MR2_TANDBERG_E_md5.txt"},
    {"MR3_TANDBERG_B/MR3_TANDBERG_B.264", Format::yuv420p, "MR3_TANDBERG_B/MR3_TANDBERG_B_md5.txt"},
    {"MR4_TANDBERG_C/MR4_TANDBERG_C.264", Format::yuv420p, "MR4_TANDBERG_C/MR4_TANDBERG_C_md5.txt"},
    {"MR5_TANDBERG_C/MR5_TANDBERG_C.264", Format::yuv420p, "MR5_TANDBERG_C/MR5_TANDBERG_C_md5.txt"},
    {"MR6_BT_B/MR6_BT_B.h264", Format::yuv420p, "MR6_BT_B/MR6_BT_B_md5.txt"},
    {"MR7_BT_B/MR7_BT_B.h264", Format::yuv420p, "MR7_BT_B/MR7_BT_B_md5.txt"},
    {"MR8_BT_B/MR8_BT_B.h264", Format::yuv420p, "MR8_BT_B/MR8_BT_B_md5.txt"},
    {"MR9_BT_B/MR9_BT_B.h264", Format::yuv420p, "MR9_BT_B/MR9_BT_B_md5.txt"},
    {"MV1_BRCM_D/src19td.IBP.264", Format::yuv420p, "MV1_BRCM_D/src19td.IBP_md5.txt"},
    {"NL1_Sony_D/NL1_Sony_D.jsv", Format::yuv420p, "NL1_Sony_D/NL1_Sony_D_md5.txt"},
    {"NL2_Sony_H/NL2_Sony_H.jsv", Format::yuv420p, "NL2_Sony_H/NL2_Sony_H_md5.txt"},
    {"NL3_SVA_E/NL3_SVA_E.264", Format::yuv420p, "NL3_SVA_E/NL3_SVA_E_md5.txt"},
    {"NLMQ1_JVC_C/NLMQ1_JVC_C.264", Format::yuv420p, "NLMQ1_JVC_C/NLMQ1_JVC_C_md5.txt"},
    {"NLMQ2_JVC_C/NLMQ2_JVC_C.264", Format::yuv420p, "NLMQ2_JVC_C/NLMQ2_JVC_C_md5.txt"},
    {"NRF_MW_E/NRF_MW_E.264", Format::yuv420p, "NRF_MW_E/NRF_MW_E_md5.txt"},
    {"SL1_SVA_B/SL1_SVA_B.264", Format::yuv420p, "SL1_SVA_B/SL1_SVA_B_md5.txt"},
    {"SVA_BA1_B/SVA_BA1_B.264", Format::yuv420p, "SVA_BA1_B/SVA_BA1_B_md5.txt"},
    {"SVA_BA2_D/SVA_BA2_D.264", Format::yuv420p, "SVA_BA2_D/SVA_BA2_D_md5.txt"},
    {"SVA_Base_B/SVA_Base_B.264", Format::yuv420p, "SVA_Base_B/SVA_Base_B_md5.txt"},
    {"SVA_CL1_E/SVA_CL1_E.264", Format::yuv420p, "SVA_CL1_E/SVA_CL1_E_md5.txt"},
    {"SVA_FM1_E/SVA_FM1_E.264", Format::yuv420p, "SVA_FM1_E/SVA_FM1_E_md5.txt"},
    {"SVA_NL1_B/SVA_NL1_B.264", Format::yuv420p, "SVA_NL1_B/SVA_NL1_B_md5.txt"},
    {"SVA_NL2_E/SVA_NL2_E.264", Format::yuv420p, "SVA_NL2_E/SVA_NL2_E_md5.txt"},
    {"Sharp_MP_Field_1_B/Sharp_MP_Field_1_B.jvt", Format::yuv420p, "Sharp_MP_Field_1_B/Sharp_MP_Field_1_B_md5.txt"},
    {"Sharp_MP_Field_2_B/Sharp_MP_Field_2_B.jvt", Format::yuv420p, "Sharp_MP_Field_2_B/Sharp_MP_Field_2_B_md5.txt"},
    {"Sharp_MP_Field_3_B/Sharp_MP_Field_3_B.jvt", Format::yuv420p, "Sharp_MP_Field_3_B/Sharp_MP_Field_3_B_md5.txt"},
    {"Sharp_MP_PAFF_1r2/Sharp_MP_PAFF_1r2.jvt", Format::yuv420p, "Sharp_MP_PAFF_1r2/Sharp_MP_PAFF_1r2_md5.txt"},
    {"Sharp_MP_PAFF_2r/Sharp_MP_PAFF_2.jvt", Format::yuv420p, "Sharp_MP_PAFF_2r/Sharp_MP_PAFF_2_md5.txt"},
    {"cabac_mot_fld0_full/camp_mot_fld0_full.26l", Format::yuv420p, "cabac_mot_fld0_full/camp_mot_fld0_full_md5.txt"},
    {"cabac_mot_frm0_full/camp_mot_frm0_full.26l", Format::yuv420p, "cabac_mot_frm0_full/camp_mot_frm0_full_md5.txt"},
    {"cabac_mot_mbaff0_full/camp_mot_mbaff0_full.26l", Format::yuv420p, "cabac_mot_mbaff0_full/camp_mot_mbaff0_full_md5.txt"},
    {"cabac_mot_picaff0_full/camp_mot_picaff0_full.26l", Format::yuv420p, "cabac_mot_picaff0_full/camp_mot_picaff0_full_md5.txt"},
    {"cama1_vtc_c/cama1_vtc_c.avc", Format::yuv420p, "cama1_vtc_c/cama1_vtc_c_md5.txt"},
    {"cama2_vtc_b/cama2_vtc_b.avc", Format::yuv420p, "cama2_vtc_b/cama2_vtc_b_md5.txt"},
    {"cama3_vtc_b/cama3_vtc_b.avc", Format::yuv420p, "cama3_vtc_b/cama3_vtc_b_md5.txt"},
    {"cavlc_mot_fld0_full_B/cvmp_mot_fld0_full_B.26l", Format::yuv420p, "cavlc_mot_fld0_full_B/cvmp_mot_fld0_full_B_md5.txt"},
    {"cavlc_mot_frm0_full_B/cvmp_mot_frm0_full_B.26l", Format::yuv420p, "cavlc_mot_frm0_full_B/cvmp_mot_frm0_full_B_md5.txt"},
    {"cavlc_mot_mbaff0_full_B/cvmp_mot_mbaff0_full_B.26l", Format::yuv420p, "cavlc_mot_mbaff0_full_B/cvmp_mot_mbaff0_full_B_md5.txt"},
    {"cavlc_mot_picaff0_full_B/cvmp_mot_picaff0_full_B.26l", Format::yuv420p, "cavlc_mot_picaff0_full_B/cvmp_mot_picaff0_full_B_md5.txt"},
    // FMO
    // {"FM1_BT_B/FM1_BT_B.h264", Format::yuv420p, "FM1_BT_B/FM1_BT_B_md5.txt"},
    // {"FM1_FT_E/FM1_FT_E.264", Format::yuv420p, "FM1_FT_E/FM1_FT_E_md5.txt"},
    // {"FM2_SVA_C/FM2_SVA_C.264", Format::yuv420p, "FM2_SVA_C/FM2_SVA_C_md5.txt"},
    // Extended Profile
    // {"SP1_BT_A/sp1_bt_a.h264", Format::yuv420p, "SP1_BT_A/sp1_bt_a_md5.txt"},
    // {"sp2_bt_b/sp2_bt_b.h264", Format::yuv420p, "sp2_bt_b/sp2_bt_b_md5.txt"},
};

TEST(DecTest, DecodeThumbnailMode) {
  for (const auto& config : kDecodeStreams) {
    std::string inputPath = getFullPath(config.inputFile);
    std::string refMd5Path = getFullPath(config.refMd5File);

    auto builder = DecHelper::Builder()
                       .setInputFilePath(inputPath)
                       .setRefMd5Path(refMd5Path)
                       .setFormat(config.format)
                       .setThumbnailMode(true);

    std::unique_ptr<DecHelper> helper = builder.build();
    ASSERT_NE(helper, nullptr)
        << "Failed to build DecHelper for input: " << inputPath;
    EXPECT_TRUE(helper->decodeFile())
        << "Decoding failed in thumbnail mode for input: " << inputPath;
  }
}

INSTANTIATE_TEST_SUITE_P(DecoderRegression, DecTestFixture,
                         ::testing::Combine(::testing::ValuesIn(kDecodeStreams),
                                            ::testing::Values(1, 2, 3)));

}  // namespace test
}  // namespace libavc
