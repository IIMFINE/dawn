#ifndef _HEAP_H_
#define _HEAP_H_
#include <algorithm>
#include <iostream>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <vector>

#include "type.h"

///@todo Improve heap multithread performance.

namespace dawn
{
#define _GET_PARENT_INDEX(index) ((index - 1) / 2)

template <typename KEY_T>
struct less
{
    /// @brief return true if a is less than b.
    /// @param a
    /// @param b
    /// @return true: a is less than b; false: a is not less than b.
    bool operator()(const KEY_T &lhs, const KEY_T &rhs) const { return lhs < rhs; }
};

template <typename KEY_T>
struct greater
{
    bool operator()(const KEY_T &a, const KEY_T &b) { return a > b; }
};

constexpr uint32_t kInvalidHeapPosition = 0xFFFFFFFF;
template <typename KEY_T, typename CONTENT_T>
struct MinixHeap
{
    struct HeapNode
    {
        struct NodeInfo : public std::enable_shared_from_this<NodeInfo>
        {
            NodeInfo() = default;
            NodeInfo(uint32_t position) : heap_position_(position) {}
            NodeInfo(const NodeInfo &) = default;

            /// @brief This destruct function will be called once only when its
            /// last shared pointer expire.
            ///        So it need to set valid_in_heap_ to false manually.
            ~NodeInfo() = default;
            uint32_t heap_position_ = kInvalidHeapPosition;
            bool valid_in_heap_ = true;
        };

        HeapNode();
        ~HeapNode() = default;
        HeapNode(uint32_t heap_index, bool valid, std::pair<KEY_T, CONTENT_T> &&data_pair);
        HeapNode(uint32_t heap_index, std::pair<KEY_T, CONTENT_T> &&data_pair);
        HeapNode(const HeapNode &node);

        /// @brief Move constructor. Transfer node_info_ pointer property to new
        /// node.
        ///        But not swap node_info_ value.
        /// @param node Move from node.
        HeapNode(HeapNode &&node);
        HeapNode &operator=(const HeapNode &node);

        /// @brief Swap node_info_ pointer property and node_info_ value. Move
        /// data_pair_ to new node.
        /// @param node: Move from node.
        /// @return HeapNode&: Reference of new node.
        HeapNode &operator=(HeapNode &&node);

        /// @brief Swap all node content to passed node.
        /// @param node passed node
        void swap(HeapNode &node);
        bool validInHeap();

        /// @brief Swap node_info_ with passed node.
        /// @param node passed node
        /// @return PROCESS_SUCCESS: true, PROCESS_FAILED: false
        bool swapInfo(HeapNode &node);

        /// @brief Modify key of node.
        /// @param key The key to apply.
        /// @return PROCESS_SUCCESS: succeed to modify key, PROCESS_FAILED:
        /// failed to modify key.
        bool modifyKey(KEY_T &&key)
        {
            if (node_info_ == nullptr)
            {
                return PROCESS_FAIL;
            }
            data_pair_->first = std::forward<KEY_T>(key);
            return PROCESS_SUCCESS;
        }

        /// @brief Get the position of node in heap.
        /// @return Position value. If node has been not in heap, it will return
        /// last position in heap.
        uint32_t getNodePosition() { return node_info_->heap_position_; }

        std::shared_ptr<std::pair<KEY_T, CONTENT_T>> getContent() { return data_pair_; }

        /// @brief Use smart pointer to keep latest heap position of node.
        std::shared_ptr<NodeInfo> node_info_;
        std::shared_ptr<std::pair<KEY_T, CONTENT_T>> data_pair_;
    };

    MinixHeap() = default;
    virtual ~MinixHeap() = default;

    static void swap(HeapNode &node_lhs, HeapNode &node_rhs);

    bool push(std::pair<KEY_T, CONTENT_T> &&data_pair);
    MinixHeap::HeapNode pushAndGetNode(std::pair<KEY_T, CONTENT_T> &&data_pair);
    std::optional<std::shared_ptr<std::pair<KEY_T, CONTENT_T>>> pop();
    std::optional<std::shared_ptr<std::pair<KEY_T, CONTENT_T>>> top();
    std::optional<typename MinixHeap::HeapNode> topHeapNode();
    bool empty();
    int size();
    bool erase(uint32_t heap_index);
    bool erase(MinixHeap::HeapNode &node);

    /// @brief Heapify from parent index.
    /// @param parent_index Given parent index.
    /// @return Return final position changed from parent index after heapify.
    virtual uint32_t heapify(uint32_t parent_index) = 0;

    /// @brief Heapify from the node in the heap when it has changed its key.
    /// @param heap_node Node changed key.
    /// @return PROCESS_SUCCESS: succeed to heapify, PROCESS_FAILED: failed to
    /// heapify.
    bool heapify(HeapNode &heap_node);

    /// @brief Heapify from child index to its parent index.
    /// @param child_index Given child index.
    /// @return Return final position changed from child index after heapify.
    virtual uint32_t heapifyFromChild(uint32_t child_index) = 0;

protected:
    uint32_t heap_size_ = 0;
    std::vector<typename MinixHeap<KEY_T, CONTENT_T>::HeapNode> heap_;
};

template <typename KEY_T, typename CONTENT_T, typename COMPARATOR = ::dawn::less<KEY_T>>
struct MaxHeap : public MinixHeap<KEY_T, CONTENT_T>
{
    using Compare = COMPARATOR;
    using HeapNode = typename MinixHeap<KEY_T, CONTENT_T>::HeapNode;
    Compare less_;

    MaxHeap() = default;
    ~MaxHeap() = default;
    MaxHeap(const MaxHeap &heap);
    MaxHeap(MaxHeap &&heap);
    MaxHeap &operator=(const MaxHeap &heap);
    MaxHeap &operator=(MaxHeap &&heap);
    using MinixHeap<KEY_T, CONTENT_T>::heapify;
    virtual uint32_t heapify(uint32_t parent_index) override;
    virtual uint32_t heapifyFromChild(uint32_t child_index) override;
};

template <typename KEY_T, typename CONTENT_T, typename COMPARATOR = ::dawn::less<KEY_T>>
struct MinHeap : public MinixHeap<KEY_T, CONTENT_T>
{
    using Compare = COMPARATOR;
    using HeapNode = typename MinixHeap<KEY_T, CONTENT_T>::HeapNode;
    Compare less_;

    MinHeap() = default;
    ~MinHeap() = default;
    MinHeap(const MinHeap &heap);
    MinHeap(MinHeap &&heap);
    MinHeap &operator=(const MinHeap &heap);
    MinHeap &operator=(MinHeap &&heap);
    using MinixHeap<KEY_T, CONTENT_T>::heapify;
    virtual uint32_t heapify(uint32_t parent_index) override;
    virtual uint32_t heapifyFromChild(uint32_t child_index) override;
};

template <typename KEY_T, typename CONTENT_T>
void MinixHeap<KEY_T, CONTENT_T>::swap(MinixHeap<KEY_T, CONTENT_T>::HeapNode &node_lhs, MinixHeap<KEY_T, CONTENT_T>::HeapNode &node_rhs)
{
    node_lhs.swapInfo(node_rhs);
    std::swap(node_lhs.node_info_, node_rhs.node_info_);
    std::swap(node_lhs.data_pair_, node_rhs.data_pair_);
}

/// @brief Push a data pair instance to heap. Thread safe: no.
/// @tparam KEY_T the type is used to determinate the order of this instance in
/// heap.
/// @tparam CONTENT_T node content.
/// @param data_pair the whole data structure want to be contained in heap.
/// @return PROCESS_SUCCESS: succeed to push to heap, PROCESS_FAILED: failed to
/// push to heap.
template <typename KEY_T, typename CONTENT_T>
bool MinixHeap<KEY_T, CONTENT_T>::push(std::pair<KEY_T, CONTENT_T> &&data_pair)
{
    this->heap_size_++;
    auto child_index = this->heap_size_ - 1;
    heap_.emplace_back(typename MinixHeap<KEY_T, CONTENT_T>::HeapNode(child_index, true, std::forward<std::pair<KEY_T, CONTENT_T>>(data_pair)));
    return (heapifyFromChild(child_index) == kInvalidHeapPosition) ? PROCESS_FAIL : PROCESS_SUCCESS;
}

template <typename KEY_T, typename CONTENT_T>
typename MinixHeap<KEY_T, CONTENT_T>::HeapNode MinixHeap<KEY_T, CONTENT_T>::pushAndGetNode(std::pair<KEY_T, CONTENT_T> &&data_pair)
{
    this->heap_size_++;
    auto child_index = this->heap_size_ - 1;
    heap_.emplace_back(typename MinixHeap<KEY_T, CONTENT_T>::HeapNode(child_index, true, std::forward<std::pair<KEY_T, CONTENT_T>>(data_pair)));
    auto resultIndex = heapifyFromChild(child_index);
    return heap_[resultIndex];
}

template <typename KEY_T, typename CONTENT_T>
std::optional<std::shared_ptr<std::pair<KEY_T, CONTENT_T>>> MinixHeap<KEY_T, CONTENT_T>::pop()
{
    if (this->heap_size_ == 0)
    {
        return std::nullopt;
    }

    heap_[0].node_info_->valid_in_heap_ = false;
    swap(heap_[0], heap_.back());
    auto ret = std::move(heap_.back());
    heap_.pop_back();
    this->heap_size_--;

    if (this->heap_size_ <= 1)
    {
        return std::make_optional(std::move((ret.data_pair_)));
    }
    else
    {
        heapify(0);
        return std::make_optional(std::move((ret.data_pair_)));
    }
}

template <typename KEY_T, typename CONTENT_T>
std::optional<std::shared_ptr<std::pair<KEY_T, CONTENT_T>>> MinixHeap<KEY_T, CONTENT_T>::top()
{
    if (this->heap_size_ == 0)
    {
        return std::nullopt;
    }
    return std::make_optional(heap_[0].data_pair_);
}

template <typename KEY_T, typename CONTENT_T>
std::optional<typename MinixHeap<KEY_T, CONTENT_T>::HeapNode> MinixHeap<KEY_T, CONTENT_T>::topHeapNode()
{
    if (this->heap_size_ == 0)
    {
        return std::nullopt;
    }
    return std::make_optional(heap_[0]);
}

template <typename KEY_T, typename CONTENT_T>
bool MinixHeap<KEY_T, CONTENT_T>::empty()
{
    return heap_size_ == 0;
}

template <typename KEY_T, typename CONTENT_T>
int MinixHeap<KEY_T, CONTENT_T>::size()
{
    return heap_size_;
}

template <typename KEY_T, typename CONTENT_T>
bool MinixHeap<KEY_T, CONTENT_T>::erase(uint32_t heap_index)
{
    if (heap_size_ == 0 || heap_[heap_index].validInHeap() == false)
    {
        return PROCESS_FAIL;
    }

    auto end_index = this->heap_size_ - 1;
    if (heap_index > end_index)
    {
        return PROCESS_FAIL;
    }
    else if (heap_index == end_index)
    {
        heap_.pop_back();
        this->heap_size_--;
        return PROCESS_SUCCESS;
    }

    heap_[heap_index].node_info_->valid_in_heap_ = false;
    swap(heap_[heap_index], heap_[end_index]);
    heap_.pop_back();
    this->heap_size_--;
    heapify(heap_index);
    return PROCESS_SUCCESS;
}

template <typename KEY_T, typename CONTENT_T>
bool MinixHeap<KEY_T, CONTENT_T>::erase(typename MinixHeap<KEY_T, CONTENT_T>::HeapNode &node)
{
    if (node.validInHeap() == false)
    {
        return PROCESS_FAIL;
    }
    return erase(node.getNodePosition());
}

template <typename KEY_T, typename CONTENT_T>
bool MinixHeap<KEY_T, CONTENT_T>::heapify(HeapNode &heap_node)
{
    if (!heap_node.validInHeap())
    {
        return PROCESS_FAIL;
    }

    heapify(heap_node.getNodePosition());
    heapifyFromChild(heap_node.getNodePosition());
    return PROCESS_SUCCESS;
}

template <typename KEY_T, typename CONTENT_T>
MinixHeap<KEY_T, CONTENT_T>::HeapNode::HeapNode() : node_info_(std::make_shared<NodeInfo>()), data_pair_(std::make_shared<std::pair<KEY_T, CONTENT_T>>())
{
}

template <typename KEY_T, typename CONTENT_T>
MinixHeap<KEY_T, CONTENT_T>::HeapNode::HeapNode(uint32_t heap_index, bool valid, std::pair<KEY_T, CONTENT_T> &&data_pair)
    : node_info_(std::make_shared<NodeInfo>(heap_index)),
      data_pair_(std::make_shared<std::pair<KEY_T, CONTENT_T>>(std::forward<std::pair<KEY_T, CONTENT_T>>(data_pair)))
{
}

template <typename KEY_T, typename CONTENT_T>
MinixHeap<KEY_T, CONTENT_T>::HeapNode::HeapNode(uint32_t heap_index, std::pair<KEY_T, CONTENT_T> &&data_pair)
    : node_info_(std::make_shared<NodeInfo>(heap_index)),
      data_pair_(std::make_shared<std::pair<KEY_T, CONTENT_T>>(std::forward<std::pair<KEY_T, CONTENT_T>>(data_pair)))
{
}

template <typename KEY_T, typename CONTENT_T>
MinixHeap<KEY_T, CONTENT_T>::HeapNode::HeapNode(const HeapNode &node) : node_info_(node.node_info_), data_pair_(node.data_pair_)
{
}

template <typename KEY_T, typename CONTENT_T>
MinixHeap<KEY_T, CONTENT_T>::HeapNode::HeapNode(HeapNode &&node) : node_info_(std::move(node.node_info_)), data_pair_(std::move(node.data_pair_))
{
}

template <typename KEY_T, typename CONTENT_T>
typename MinixHeap<KEY_T, CONTENT_T>::HeapNode &MinixHeap<KEY_T, CONTENT_T>::HeapNode::operator=(const HeapNode &node)
{
    data_pair_ = node.data_pair_;
    node_info_ = node.node_info_;
    return *this;
}

template <typename KEY_T, typename CONTENT_T>
typename MinixHeap<KEY_T, CONTENT_T>::HeapNode &MinixHeap<KEY_T, CONTENT_T>::HeapNode::operator=(HeapNode &&node)
{
    node_info_ = std::move(node.node_info_);
    data_pair_ = std::move(node.data_pair_);
    return *this;
}

template <typename KEY_T, typename CONTENT_T>
void MinixHeap<KEY_T, CONTENT_T>::HeapNode::swap(HeapNode &node)
{
    std::swap(node.data_pair_, data_pair_);
    std::swap(node_info_, node.node_info_);
}

template <typename KEY_T, typename CONTENT_T>
bool MinixHeap<KEY_T, CONTENT_T>::HeapNode::validInHeap()
{
    return node_info_->valid_in_heap_;
}

template <typename KEY_T, typename CONTENT_T>
bool MinixHeap<KEY_T, CONTENT_T>::HeapNode::swapInfo(HeapNode &node)
{
    if (node.node_info_ == nullptr || node_info_ == nullptr)
    {
        return PROCESS_FAIL;
    }

    std::swap(*node_info_, *node.node_info_);

    return PROCESS_SUCCESS;
}

template <typename KEY_T, typename CONTENT_T, typename COMPARATOR>
MaxHeap<KEY_T, CONTENT_T, COMPARATOR>::MaxHeap(const MaxHeap &heap) : MinixHeap<KEY_T, CONTENT_T>()
{
    this->heap_ = heap.heap_;
    this->heap_size_ = heap.heap_size_;
}

template <typename KEY_T, typename CONTENT_T, typename COMPARATOR>
MaxHeap<KEY_T, CONTENT_T, COMPARATOR>::MaxHeap(MaxHeap &&heap) : MinixHeap<KEY_T, CONTENT_T>()
{
    this->heap_ = std::move(heap.heap_);
    this->heap_size_ = heap.heap_size_;
    heap.heap_size_ = 0;
}

template <typename KEY_T, typename CONTENT_T, typename COMPARATOR>
MaxHeap<KEY_T, CONTENT_T, COMPARATOR> &MaxHeap<KEY_T, CONTENT_T, COMPARATOR>::operator=(const MaxHeap &heap)
{
    this->heap_ = heap.heap_;
    this->heap_size_ = heap.heap_size_;
    return *this;
}

template <typename KEY_T, typename CONTENT_T, typename COMPARATOR>
MaxHeap<KEY_T, CONTENT_T, COMPARATOR> &MaxHeap<KEY_T, CONTENT_T, COMPARATOR>::operator=(MaxHeap &&heap)
{
    this->heap_ = std::move(heap.heap_);
    this->heap_size_ = heap.heap_size_;
    heap.heap_size_ = 0;
    return *this;
}

template <typename KEY_T, typename CONTENT_T, typename COMPARATOR>
uint32_t MaxHeap<KEY_T, CONTENT_T, COMPARATOR>::heapify(uint32_t parent_index)
{
    auto end_index = this->heap_size_ - 1;
    while (1)
    {
        uint32_t left_child_index = 2 * parent_index + 1;
        uint32_t right_child_index = 2 * parent_index + 2;

        if (right_child_index > end_index)
        {
            right_child_index = end_index;
            if (left_child_index > end_index)
            {
                break;
            }
        }

        if (less_(this->heap_[left_child_index].data_pair_->first, this->heap_[right_child_index].data_pair_->first))
        {
            if (less_(this->heap_[parent_index].data_pair_->first, this->heap_[right_child_index].data_pair_->first))
            {
                MinixHeap<KEY_T, CONTENT_T>::swap(this->heap_[right_child_index], this->heap_[parent_index]);
                parent_index = right_child_index;
            }
            else
            {
                break;
            }
        }
        else
        {
            if (less_(this->heap_[parent_index].data_pair_->first, this->heap_[left_child_index].data_pair_->first))
            {
                MinixHeap<KEY_T, CONTENT_T>::swap(this->heap_[left_child_index], this->heap_[parent_index]);
                parent_index = left_child_index;
            }
            else
            {
                break;
            }
        }

        if (parent_index == end_index)
        {
            break;
        }
    }
    return parent_index;
}

template <typename KEY_T, typename CONTENT_T, typename COMPARATOR>
uint32_t MaxHeap<KEY_T, CONTENT_T, COMPARATOR>::heapifyFromChild(uint32_t child_index)
{
    while (1)
    {
        if (child_index == 0)
        {
            break;
        }
        auto parent_index = _GET_PARENT_INDEX(child_index);
        if (less_(this->heap_[parent_index].data_pair_->first, this->heap_[child_index].data_pair_->first))
        {
            MinixHeap<KEY_T, CONTENT_T>::swap(this->heap_[parent_index], this->heap_[child_index]);
            child_index = parent_index;
        }
        else
        {
            break;
        }
    }
    return child_index;
}

template <typename KEY_T, typename CONTENT_T, typename COMPARATOR>
MinHeap<KEY_T, CONTENT_T, COMPARATOR>::MinHeap(const MinHeap &heap) : MinixHeap<KEY_T, CONTENT_T>()
{
    this->heap_ = heap.heap_;
    this->heap_size_ = heap.heap_size_;
}

template <typename KEY_T, typename CONTENT_T, typename COMPARATOR>
MinHeap<KEY_T, CONTENT_T, COMPARATOR>::MinHeap(MinHeap &&heap) : MinixHeap<KEY_T, CONTENT_T>()
{
    this->heap_ = std::move(heap.heap_);
    this->heap_size_ = heap.heap_size_;
    heap.heap_size_ = 0;
}

template <typename KEY_T, typename CONTENT_T, typename COMPARATOR>
MinHeap<KEY_T, CONTENT_T, COMPARATOR> &MinHeap<KEY_T, CONTENT_T, COMPARATOR>::operator=(const MinHeap &heap)
{
    this->heap_ = heap.heap_;
    this->heap_size_ = heap.heap_size_;
    return *this;
}

template <typename KEY_T, typename CONTENT_T, typename COMPARATOR>
MinHeap<KEY_T, CONTENT_T, COMPARATOR> &MinHeap<KEY_T, CONTENT_T, COMPARATOR>::operator=(MinHeap &&heap)
{
    this->heap_ = std::move(heap.heap_);
    this->heap_size_ = heap.heap_size_;
    heap.heap_size_ = 0;
    return *this;
}

template <typename KEY_T, typename CONTENT_T, typename COMPARATOR>
uint32_t MinHeap<KEY_T, CONTENT_T, COMPARATOR>::heapify(uint32_t parent_index)
{
    auto end_index = this->heap_size_ - 1;
    while (1)
    {
        uint32_t left_child_index = 2 * parent_index + 1;
        uint32_t right_child_index = 2 * parent_index + 2;

        if (right_child_index > end_index)
        {
            right_child_index = end_index;
            if (left_child_index > end_index)
            {
                break;
            }
        }

        if (less_(this->heap_[right_child_index].data_pair_->first, this->heap_[left_child_index].data_pair_->first))
        {
            if (less_(this->heap_[right_child_index].data_pair_->first, this->heap_[parent_index].data_pair_->first))
            {
                MinixHeap<KEY_T, CONTENT_T>::swap(this->heap_[right_child_index], this->heap_[parent_index]);
                parent_index = right_child_index;
            }
            else
            {
                break;
            }
        }
        else
        {
            if (less_(this->heap_[left_child_index].data_pair_->first, this->heap_[parent_index].data_pair_->first))
            {
                MinixHeap<KEY_T, CONTENT_T>::swap(this->heap_[left_child_index], this->heap_[parent_index]);
                parent_index = left_child_index;
            }
            else
            {
                break;
            }
        }
        if (parent_index == end_index)
        {
            break;
        }
    }

    return parent_index;
}

template <typename KEY_T, typename CONTENT_T, typename COMPARATOR>
uint32_t MinHeap<KEY_T, CONTENT_T, COMPARATOR>::heapifyFromChild(uint32_t child_index)
{
    while (1)
    {
        if (child_index == 0)
        {
            break;
        }
        auto parent_index = _GET_PARENT_INDEX(child_index);
        if (less_(this->heap_[child_index].data_pair_->first, this->heap_[parent_index].data_pair_->first))
        {
            MinixHeap<KEY_T, CONTENT_T>::swap(this->heap_[parent_index], this->heap_[child_index]);
            child_index = parent_index;
        }
        else
        {
            break;
        }
    }
    return child_index;
}

}  // namespace dawn

#endif
