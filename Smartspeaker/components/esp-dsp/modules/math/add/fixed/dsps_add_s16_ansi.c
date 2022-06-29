// Copyright 2018-2019 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "dsps_add.h"

esp_err_t dsps_add_s16_ansi(const int16_t *input1, const int16_t *input2, int16_t *output, int len, int step1, int step2, int step_out, int shift)
{
    if (NULL == input1) return ESP_ERR_DSP_PARAM_OUTOFRANGE;
    if (NULL == input2) return ESP_ERR_DSP_PARAM_OUTOFRANGE;
    if (NULL == output) return ESP_ERR_DSP_PARAM_OUTOFRANGE;

    for (int i = 0 ; i < len ; i++) {
        int32_t acc = (int32_t)input1[i * step1] + (int32_t)input2[i * step2];
        output[i * step_out] = acc >> shift;
    }
    return ESP_OK;
}
