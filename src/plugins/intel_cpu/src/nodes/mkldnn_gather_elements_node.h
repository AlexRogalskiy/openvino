// Copyright (C) 2018-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <ie_common.h>
#include <mkldnn_node.h>
#include <string>
#include <memory>
#include <vector>

namespace MKLDNNPlugin {

class MKLDNNGatherElementsNode : public MKLDNNNode {
public:
    MKLDNNGatherElementsNode(const std::shared_ptr<ngraph::Node>& op, const mkldnn::engine& eng, MKLDNNWeightsSharing::Ptr &cache);

    void getSupportedDescriptors() override {};
    void initSupportedPrimitiveDescriptors() override;
    void execute(mkldnn::stream strm) override;
    bool created() const override;

    static bool isSupportedOperation(const std::shared_ptr<const ov::Node>& op, std::string& errorMessage) noexcept;

protected:
    void executeDynamicImpl(mkldnn::stream strm) override;
    void prepareParams() override;

private:
    const size_t dataIndex_ = 0;
    const size_t indicesIndex_ = 1;

    size_t axis_;
    size_t dataTypeSize_ = 0;
    int strideAxDst_ = 0;
    int dstAxDim_ = 0;
    int strideAx1Diff_ = 0;
    std::string errorPrefix_;

    template <typename dataType>
    void directExecution();
};

}  // namespace MKLDNNPlugin
