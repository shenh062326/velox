# Copyright (c) Facebook, Inc. and its affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
include(GoogleTest)
add_executable(velox_test_util_test TestValueTest.cpp SpillConfigTest.cpp)
gtest_add_tests(velox_test_util_test "" AUTO)

target_link_libraries(
  velox_test_util_test
  PRIVATE
  velox_test_util
  velox_exception
  velox_spill_config
  velox_exec
  gtest
  gtest_main)
