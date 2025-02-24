// Copyright (C) 2018-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "blocked_memory_desc.h"
#include "utils/general_utils.h"

namespace MKLDNNPlugin {

class CpuBlockedMemoryDesc : public BlockedMemoryDesc {
public:
    CpuBlockedMemoryDesc(InferenceEngine::Precision prc, const Shape& shape);

    CpuBlockedMemoryDesc(InferenceEngine::Precision prc, const Shape& shape, const VectorDims& blockedDims,
                         const VectorDims& order, size_t offsetPadding = 0, const VectorDims& offsetPaddingToData = {},
                         const VectorDims& strides = {});

    MemoryDescPtr clone() const override {
        return std::make_shared<CpuBlockedMemoryDesc>(*this);
    }

    bool isCompatible(const MemoryDesc& rhs) const override;
    bool isCompatible(const BlockedMemoryDesc& rhs, CmpMask cmpMask) const override;
    bool isCompatible(const CpuBlockedMemoryDesc &rhs, CmpMask cmpMask = BLOCKED_DESC_FULL_MASK) const;
    bool isCompatible(const DnnlBlockedMemoryDesc &rhs, CmpMask cmpMask = BLOCKED_DESC_FULL_MASK) const;

    InferenceEngine::Precision getPrecision() const override {
        return precision;
    }

    const VectorDims& getBlockDims() const override {
        return blockedDims;
    }

    /**
     * @brief Returns the vector of order
     *
     * @return order
     */
    const VectorDims& getOrder() const override {
        return order;
    }

    /**
     * @brief Returns the per-dimension offset vector
     *
     * @return offsets
     */
    const VectorDims& getOffsetPaddingToData() const override {
        return offsetPaddingToData;
    }
    /**
     * @brief Returns the offset to the current memory block
     *
     * @return offset
     */
    size_t getOffsetPadding() const override {
        return offsetPadding;
    }

    /**
     * @brief Returns strides for each dimension
     *
     * @return strides
     */
    const VectorDims& getStrides() const override {
        return strides;
    }

    bool blocksExtended() const override;

    bool hasLayoutType(LayoutType layoutType) const override;

    size_t getMaxMemSize() const override;

    size_t getPaddedElementsCount() const override;

    MemoryDescPtr cloneWithNewPrecision(const InferenceEngine::Precision prec) const override;

private:
    size_t getElementOffset(size_t elemNumber) const override;
    bool canComputeMemSizeZeroDims() const override;
    size_t getCurrentMemSizeImp() const override;
    size_t getOffset(const InferenceEngine::SizeVector& v) const;
    bool isPlainFormat() const;
    bool isBlockedCFormat(size_t blk_size) const;
    bool isTailCFormat() const;
    bool isDefinedImp() const override;
    MemoryDescPtr cloneWithNewDimsImp(const VectorDims& dims) const override;

    void setPrecision(InferenceEngine::Precision prc) override {
        precision = std::move(prc);
    }

private:
    InferenceEngine::Precision precision;
    size_t offsetPadding;
};

using CpuBlockedMemoryDescPtr = std::shared_ptr<CpuBlockedMemoryDesc>;
using CpuBlockedMemoryDescCPtr = std::shared_ptr<const CpuBlockedMemoryDesc>;

} // namespace MKLDNNPlugin
