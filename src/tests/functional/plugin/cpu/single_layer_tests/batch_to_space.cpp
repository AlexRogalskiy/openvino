// Copyright (C) 2018-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include "shared_test_classes/base/ov_subgraph.hpp"
#include "ngraph_functions/builders.hpp"
#include "test_utils/cpu_test_utils.hpp"

using namespace InferenceEngine;
using namespace CPUTestUtils;
using namespace ngraph::opset3;
using namespace ov::test;

namespace CPULayerTestsDefinitions  {

using BatchToSpaceLayerTestCPUParams = std::tuple<
        std::vector<InputShape>,            // Input shapes
        std::vector<int64_t>,               // block shape
        std::vector<int64_t>,               // crops begin
        std::vector<int64_t>,               // crops end
        Precision ,                         // Network precision
        CPUSpecificParams>;

class BatchToSpaceCPULayerTest : public testing::WithParamInterface<BatchToSpaceLayerTestCPUParams>,
                                 virtual public SubgraphBaseTest, public CPUTestsBase {
public:
    static std::string getTestCaseName(const testing::TestParamInfo<BatchToSpaceLayerTestCPUParams> &obj) {
        std::vector<InputShape> inputShapes;
        std::vector<int64_t> blockShape, cropsBegin, cropsEnd;
        Precision netPrecision;
        CPUSpecificParams cpuParams;
        std::tie(inputShapes, blockShape, cropsBegin, cropsEnd, netPrecision, cpuParams) = obj.param;
        std::ostringstream result;
        if (inputShapes.front().first.size() != 0) {
            result << "IS=(";
            for (const auto &shape : inputShapes) {
                result << CommonTestUtils::partialShape2str({shape.first}) << "_";
            }
            result.seekp(-1, result.cur);
            result << ")_";
        }
        result << "TS=";
        for (const auto& shape : inputShapes) {
            for (const auto& item : shape.second) {
                result << CommonTestUtils::vec2str(item) << "_";
            }
        }
        result << "blockShape=" << CommonTestUtils::vec2str(blockShape) << "_";
        result << "cropsBegin=" << CommonTestUtils::vec2str(cropsBegin) << "_";
        result << "cropsEnd=" << CommonTestUtils::vec2str(cropsEnd) << "_";
        result << "netPRC=" << netPrecision.name() << "_";
        result << CPUTestsBase::getTestCaseName(cpuParams);
        return result.str();
    }

protected:
    void SetUp() override {
        targetDevice = CommonTestUtils::DEVICE_CPU;

        std::vector<InputShape>  inputShapes;
        std::vector<int64_t> blockShape, cropsBegin, cropsEnd;
        Precision netPrecision;
        CPUSpecificParams cpuParams;
        std::tie(inputShapes, blockShape, cropsBegin, cropsEnd, netPrecision, cpuParams) = this->GetParam();
        std::tie(inFmts, outFmts, priority, selectedType) = cpuParams;

        auto ngPrec = FuncTestUtils::PrecisionUtils::convertIE2nGraphPrc(netPrecision);
        inType = outType = ngPrec;

        init_input_shapes(inputShapes);

        if (strcmp(netPrecision.name(), "U8") == 0)
            selectedType = std::string("ref_any_") + "I8";
        else
            selectedType = std::string("ref_any_") + netPrecision.name();

        auto params = ngraph::builder::makeDynamicParams(ngPrec, {inputDynamicShapes.front()});
        auto paramOuts = ngraph::helpers::convert2OutputVector(ngraph::helpers::castOps2Nodes<ngraph::op::Parameter>(params));
        auto b2s = ngraph::builder::makeBatchToSpace(paramOuts[0], ngPrec, blockShape, cropsBegin, cropsEnd);
        b2s->get_rt_info() = getCPUInfo();
        ngraph::ResultVector results{std::make_shared<ngraph::opset1::Result>(b2s)};
        function = std::make_shared<ngraph::Function>(results, params, "BatchToSpace");
    }
};

TEST_P(BatchToSpaceCPULayerTest, CompareWithRefs) {
    SKIP_IF_CURRENT_TEST_IS_DISABLED()

    run();
    CheckPluginRelatedResults(executableNetwork, "BatchToSpace");
};

namespace {

const std::vector<Precision> netPrecision = {
        Precision::U8,
        Precision::I8,
        Precision::I32,
        Precision::FP32,
        Precision::BF16
};

const std::vector<std::vector<int64_t>> blockShape4D1  = {{1, 1, 1, 2}, {1, 2, 2, 1}};
const std::vector<std::vector<int64_t>> cropsBegin4D1  = {{0, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 2, 0}};
const std::vector<std::vector<int64_t>> cropsEnd4D1    = {{0, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 1, 1}};

std::vector<std::vector<ov::Shape>> staticInputShapes4D1 = {
        {{8, 16, 10, 10}}
};

std::vector<std::vector<InputShape>> dynamicInputShapes4D1 = {
        {
                {{{-1, -1, -1, -1}, {{8, 8, 6, 7}, {4, 10, 5, 5}, {12, 9, 7, 5}}}},
                {{{{4, 12}, {8, 16}, 6, -1}, {{8, 8, 6, 7}, {4, 10, 6, 5}, {12, 9, 6, 5}}}}
        }
};

std::vector<std::vector<InputShape>> dynamicInputShapes4D1Blocked = {
        {
                {{{-1, 16, -1, -1}, {{4, 16, 5, 8}, {8, 16, 7, 6}, {12, 16, 4, 5}}}}
        }
};

const std::vector<std::vector<int64_t>> blockShape4D2  = {{1, 2, 3, 4}, {1, 3, 4, 2}};
const std::vector<std::vector<int64_t>> cropsBegin4D2  = {{0, 0, 0, 1}, {0, 0, 1, 2}};
const std::vector<std::vector<int64_t>> cropsEnd4D2    = {{0, 0, 1, 0}, {0, 0, 3, 1}};

std::vector<std::vector<ov::Shape>> staticInputShapes4D2 = {
        {{24, 16, 7, 8}}
};

std::vector<std::vector<InputShape>> dynamicInputShapes4D2 = {
        {
                {{{-1, -1, -1, -1}, {{48, 4, 7, 8}, {24, 8, 6, 7}, {24, 16, 5, 5}}}},
                 {{{24, {4, 10}, -1, -1}, {{24, 8, 6, 7}, {24, 6, 7, 5}, {24, 4, 5, 5}}}}
        }
};

std::vector<std::vector<InputShape>> dynamicInputShapes4D2Blocked = {
        {
                 {{-1, 16, -1, -1}, {{24, 16, 5, 5}, {24, 16, 6, 7}, {48, 16, 4, 4}}}
        }
};

const std::vector<CPUSpecificParams> cpuParamsWithBlock_4D = {
        CPUSpecificParams({nChw16c}, {nChw16c}, {}, {}),
        CPUSpecificParams({nChw8c}, {nChw8c}, {}, {}),
        CPUSpecificParams({nhwc}, {nhwc}, {}, {}),
        CPUSpecificParams({nchw}, {nchw}, {}, {})
};

const std::vector<CPUSpecificParams> cpuParams_4D = {
        CPUSpecificParams({nhwc}, {nhwc}, {}, {}),
        CPUSpecificParams({nchw}, {nchw}, {}, {})
};

const auto staticBatchToSpaceParamsSet4D1 = ::testing::Combine(
        ::testing::ValuesIn(static_shapes_to_test_representation(staticInputShapes4D1)),
        ::testing::ValuesIn(blockShape4D1),
        ::testing::ValuesIn(cropsBegin4D1),
        ::testing::ValuesIn(cropsEnd4D1),
        ::testing::ValuesIn(netPrecision),
        ::testing::ValuesIn(cpuParamsWithBlock_4D));

const auto dynamicBatchToSpaceParamsSet4D1 = ::testing::Combine(
        ::testing::ValuesIn(dynamicInputShapes4D1),
        ::testing::ValuesIn(blockShape4D1),
        ::testing::ValuesIn(cropsBegin4D1),
        ::testing::ValuesIn(cropsEnd4D1),
        ::testing::ValuesIn(netPrecision),
        ::testing::ValuesIn(cpuParams_4D));

const auto dynamicBatchToSpaceParamsWithBlockedSet4D1 = ::testing::Combine(
        ::testing::ValuesIn(dynamicInputShapes4D1Blocked),
        ::testing::ValuesIn(blockShape4D1),
        ::testing::ValuesIn(cropsBegin4D1),
        ::testing::ValuesIn(cropsEnd4D1),
        ::testing::ValuesIn(netPrecision),
        ::testing::ValuesIn(cpuParamsWithBlock_4D));

const auto staticBatchToSpaceParamsSet4D2 = ::testing::Combine(
        ::testing::ValuesIn(static_shapes_to_test_representation(staticInputShapes4D2)),
        ::testing::ValuesIn(blockShape4D2),
        ::testing::ValuesIn(cropsBegin4D2),
        ::testing::ValuesIn(cropsEnd4D2),
        ::testing::ValuesIn(netPrecision),
        ::testing::ValuesIn(cpuParamsWithBlock_4D));

const auto dynamicBatchToSpaceParamsSet4D2 = ::testing::Combine(
        ::testing::ValuesIn(dynamicInputShapes4D2),
        ::testing::ValuesIn(blockShape4D2),
        ::testing::ValuesIn(cropsBegin4D2),
        ::testing::ValuesIn(cropsEnd4D2),
        ::testing::ValuesIn(netPrecision),
        ::testing::ValuesIn(cpuParams_4D));

const auto dynamicBatchToSpaceParamsWithBlockedSet4D2 = ::testing::Combine(
        ::testing::ValuesIn(dynamicInputShapes4D2Blocked),
        ::testing::ValuesIn(blockShape4D2),
        ::testing::ValuesIn(cropsBegin4D2),
        ::testing::ValuesIn(cropsEnd4D2),
        ::testing::ValuesIn(netPrecision),
        ::testing::ValuesIn(cpuParamsWithBlock_4D));

INSTANTIATE_TEST_SUITE_P(smoke_StaticBatchToSpaceCPULayerTestCase1_4D, BatchToSpaceCPULayerTest,
                         staticBatchToSpaceParamsSet4D1, BatchToSpaceCPULayerTest::getTestCaseName);

INSTANTIATE_TEST_SUITE_P(smoke_DynamicBatchToSpaceCPULayerTestCase1_4D, BatchToSpaceCPULayerTest,
                         dynamicBatchToSpaceParamsSet4D1, BatchToSpaceCPULayerTest::getTestCaseName);

INSTANTIATE_TEST_SUITE_P(smoke_DynamicBatchToSpaceCPULayerTestCaseWithBlocked1_4D, BatchToSpaceCPULayerTest,
                         dynamicBatchToSpaceParamsWithBlockedSet4D1, BatchToSpaceCPULayerTest::getTestCaseName);

INSTANTIATE_TEST_SUITE_P(smoke_StaticBatchToSpaceCPULayerTestCase2_4D, BatchToSpaceCPULayerTest,
                         staticBatchToSpaceParamsSet4D2, BatchToSpaceCPULayerTest::getTestCaseName);

INSTANTIATE_TEST_SUITE_P(smoke_DynamicBatchToSpaceCPULayerTestCase2_4D, BatchToSpaceCPULayerTest,
                         dynamicBatchToSpaceParamsSet4D2, BatchToSpaceCPULayerTest::getTestCaseName);

INSTANTIATE_TEST_SUITE_P(smoke_DynamicBatchToSpaceCPULayerTestCaseWithBlocked2_4D, BatchToSpaceCPULayerTest,
                         dynamicBatchToSpaceParamsWithBlockedSet4D2, BatchToSpaceCPULayerTest::getTestCaseName);

const std::vector<std::vector<int64_t>> blockShape5D1  = {{1, 1, 2, 2, 1}, {1, 2, 1, 2, 2}};
const std::vector<std::vector<int64_t>> cropsBegin5D1  = {{0, 0, 0, 0, 0}, {0, 0, 0, 3, 3}};
const std::vector<std::vector<int64_t>> cropsEnd5D1    = {{0, 0, 0, 0, 0}, {0, 0, 1, 0, 1}};

std::vector<std::vector<ov::Shape>> staticInputShapes5D1 = {
        {{8, 16, 4, 10, 10}}
};

std::vector<std::vector<InputShape>> dynamicInputShapes5D1 = {
        {
                {{{-1, -1, -1, -1, -1}, {{8, 16, 4, 10, 10}, {16, 10, 5, 11, 9}, {24, 6, 6, 8, 8}}}},
                {{{{8, 16}, {8, 16}, {2, 7}, -1, -1}, {{8, 16, 2, 6, 8}, {8, 10, 4, 7, 5}, {16, 8, 7, 5, 10}}}}
        }
};

std::vector<std::vector<InputShape>> dynamicInputShapes5D1Blocked = {
        {
                {{{-1, 16, -1, -1, -1}, {{24, 16, 3, 6, 7}, {48, 16, 4, 5, 5}, {24, 16, 5, 8, 5}}}}
        }
};

const std::vector<std::vector<int64_t>> blockShape5D2  = {{1, 2, 4, 3, 1}, {1, 1, 2, 4, 3}};
const std::vector<std::vector<int64_t>> cropsBegin5D2  = {{0, 0, 1, 2, 0}, {0, 0, 1, 0, 1}};
const std::vector<std::vector<int64_t>> cropsEnd5D2    = {{0, 0, 1, 0, 1}, {0, 0, 1, 1, 1}};

std::vector<std::vector<ov::Shape>> staticInputShapes5D2 = {
        {{48, 16, 3, 3, 3}}
};

std::vector<std::vector<InputShape>> dynamicInputShapes5D2 = {
        {
                {{{-1, -1, -1, -1, -1}, {{48, 4, 3, 3, 3}, {24, 16, 5, 3, 5}, {24, 8, 7, 5, 5}}}},
                {{{24, {8, 16}, {3, 5}, -1, -1}, {{24, 16, 3, 4, 3}, {24, 12, 5, 3, 5}, {24, 8, 4, 5, 5}}}},
                // special case
                {
                    {{{1, 24}, {1, 16}, {1, 10}, {1, 10}, {1, 10}},
                    {
                        {24, 16, 5, 3, 5},
                        {24, 16, 5, 3, 5},
                        {24, 16, 7, 5, 5}
                    }}
                }
        }
};

std::vector<std::vector<InputShape>> dynamicInputShapes5D2Blocked = {
        {
                {{{-1, 16, -1, -1, -1}, {{24, 16, 4, 5, 5}, {48, 16, 3, 4, 3}, {24, 16, 5, 3, 5}}}}
        }
};

const std::vector<CPUSpecificParams> cpuParamsWithBlock_5D = {
        CPUSpecificParams({nCdhw16c}, {nCdhw16c}, {}, {}),
        CPUSpecificParams({nCdhw8c}, {nCdhw8c}, {}, {}),
        CPUSpecificParams({ndhwc}, {ndhwc}, {}, {}),
        CPUSpecificParams({ncdhw}, {ncdhw}, {}, {})
};

const std::vector<CPUSpecificParams> cpuParams_5D = {
        CPUSpecificParams({ndhwc}, {ndhwc}, {}, {}),
        CPUSpecificParams({ncdhw}, {ncdhw}, {}, {})
};

const auto staticBatchToSpaceParamsSet5D1 = ::testing::Combine(
        ::testing::ValuesIn(static_shapes_to_test_representation(staticInputShapes5D1)),
        ::testing::ValuesIn(blockShape5D1),
        ::testing::ValuesIn(cropsBegin5D1),
        ::testing::ValuesIn(cropsEnd5D1),
        ::testing::ValuesIn(netPrecision),
        ::testing::ValuesIn(cpuParamsWithBlock_5D));

const auto dynamicBatchToSpaceParamsSet5D1 = ::testing::Combine(
        ::testing::ValuesIn(dynamicInputShapes5D1),
        ::testing::ValuesIn(blockShape5D1),
        ::testing::ValuesIn(cropsBegin5D1),
        ::testing::ValuesIn(cropsEnd5D1),
        ::testing::ValuesIn(netPrecision),
        ::testing::ValuesIn(cpuParams_5D));

const auto dynamicBatchToSpaceParamsWithBlockedSet5D1 = ::testing::Combine(
        ::testing::ValuesIn(dynamicInputShapes5D1Blocked),
        ::testing::ValuesIn(blockShape5D1),
        ::testing::ValuesIn(cropsBegin5D1),
        ::testing::ValuesIn(cropsEnd5D1),
        ::testing::ValuesIn(netPrecision),
        ::testing::ValuesIn(cpuParamsWithBlock_5D));

const auto staticBatchToSpaceParamsSet5D2 = ::testing::Combine(
        ::testing::ValuesIn(static_shapes_to_test_representation(staticInputShapes5D2)),
        ::testing::ValuesIn(blockShape5D2),
        ::testing::ValuesIn(cropsBegin5D2),
        ::testing::ValuesIn(cropsEnd5D2),
        ::testing::ValuesIn(netPrecision),
        ::testing::ValuesIn(cpuParamsWithBlock_5D));

const auto dynamicBatchToSpaceParamsSet5D2 = ::testing::Combine(
        ::testing::ValuesIn(dynamicInputShapes5D2),
        ::testing::ValuesIn(blockShape5D2),
        ::testing::ValuesIn(cropsBegin5D2),
        ::testing::ValuesIn(cropsEnd5D2),
        ::testing::ValuesIn(netPrecision),
        ::testing::ValuesIn(cpuParams_5D));

const auto dynamicBatchToSpaceParamsWithBlockedSet5D2 = ::testing::Combine(
        ::testing::ValuesIn(dynamicInputShapes5D2Blocked),
        ::testing::ValuesIn(blockShape5D2),
        ::testing::ValuesIn(cropsBegin5D2),
        ::testing::ValuesIn(cropsEnd5D2),
        ::testing::ValuesIn(netPrecision),
        ::testing::ValuesIn(cpuParamsWithBlock_5D));

INSTANTIATE_TEST_SUITE_P(smoke_StaticBatchToSpaceCPULayerTestCase1_5D, BatchToSpaceCPULayerTest,
                         staticBatchToSpaceParamsSet5D1, BatchToSpaceCPULayerTest::getTestCaseName);

INSTANTIATE_TEST_SUITE_P(smoke_DynamicBatchToSpaceCPULayerTestCase1_5D, BatchToSpaceCPULayerTest,
                         dynamicBatchToSpaceParamsSet5D1, BatchToSpaceCPULayerTest::getTestCaseName);

INSTANTIATE_TEST_SUITE_P(smoke_DynamicBatchToSpaceCPULayerTestCaseWithBlocked1_5D, BatchToSpaceCPULayerTest,
                         dynamicBatchToSpaceParamsWithBlockedSet5D1, BatchToSpaceCPULayerTest::getTestCaseName);

INSTANTIATE_TEST_SUITE_P(smoke_StaticBatchToSpaceCPULayerTestCase2_5D, BatchToSpaceCPULayerTest,
                         staticBatchToSpaceParamsSet5D2, BatchToSpaceCPULayerTest::getTestCaseName);

INSTANTIATE_TEST_SUITE_P(smoke_DynamicBatchToSpaceCPULayerTestCase2_5D, BatchToSpaceCPULayerTest,
                         dynamicBatchToSpaceParamsSet5D2, BatchToSpaceCPULayerTest::getTestCaseName);

INSTANTIATE_TEST_SUITE_P(smoke_DynamicBatchToSpaceCPULayerTestCaseWithBlocked2_5D, BatchToSpaceCPULayerTest,
                         dynamicBatchToSpaceParamsWithBlockedSet5D2, BatchToSpaceCPULayerTest::getTestCaseName);

}  // namespace
}  // namespace CPULayerTestsDefinitions
