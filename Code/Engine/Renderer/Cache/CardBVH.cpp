#include "CardBVH.h"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/MathUtils.hpp"
#include <algorithm>

// ========================================
// 构建 BVH
// ========================================

void CardBVH::Build(const std::vector<SurfaceCardMetadata>& cards)
{
    Clear();
    
    if (cards.empty())
    {
        DebuggerPrintf("[CardBVH] Warning: No cards to build BVH\n");
        return;
    }
    
    m_cards = &cards;
    m_maxDepth = 0;
    m_nodeCount = 0;
    m_leafCount = 0;
    
    // 创建所有 Card 的索引列表
    std::vector<uint32_t> allCardIndices;
    allCardIndices.reserve(cards.size());
    for (size_t i = 0; i < cards.size(); i++)
    {
        allCardIndices.push_back(static_cast<uint32_t>(i));
    }
    
    // 递归构建
    m_root = BuildRecursive(allCardIndices, 0);
    
    DebuggerPrintf("[CardBVH] Built: %zu cards, %d nodes, %d leafs, max depth %d\n",
                   cards.size(), m_nodeCount, m_leafCount, m_maxDepth);
    
    if (m_root)
    {
        Vec3 boundsSize = m_root->m_bounds.GetBoundsSize();
        DebuggerPrintf("[CardBVH] Root bounds: size=(%.2f, %.2f, %.2f)\n",
                       boundsSize.x, boundsSize.y, boundsSize.z);
    }
}

void CardBVH::Clear()
{
    m_root.reset();
    m_cards = nullptr;
    m_maxDepth = 0;
    m_nodeCount = 0;
    m_leafCount = 0;
}

std::unique_ptr<CardBVHNode> CardBVH::BuildRecursive(
    const std::vector<uint32_t>& cardIndices,
    int depth)
{
    if (!m_cards || cardIndices.empty())
        return nullptr;
    
    auto node = std::make_unique<CardBVHNode>();
    m_nodeCount++;
    m_maxDepth = MaxI(m_maxDepth, depth);
    
    // 计算节点的 AABB
    node->m_bounds = ComputeBounds(cardIndices);
    
    // 终止条件：Card 数量少或深度过大
    if (cardIndices.size() <= MAX_CARDS_PER_LEAF || depth >= MAX_DEPTH)
    {
        node->m_isLeaf = true;
        node->m_cardIndices = cardIndices;
        m_leafCount++;
        return node;
    }
    
    // 选择分割轴
    int axis = ChooseSplitAxis(cardIndices);
    
    // 按照中心点排序
    std::vector<uint32_t> sortedIndices = cardIndices;
    std::sort(sortedIndices.begin(), sortedIndices.end(),
        [&](uint32_t a, uint32_t b) {
            Vec3 centerA = GetCardCenter(a);
            Vec3 centerB = GetCardCenter(b);
            
            if (axis == 0) return centerA.x < centerB.x;
            if (axis == 1) return centerA.y < centerB.y;
            return centerA.z < centerB.z;
        });
    
    // 分割成两组
    size_t mid = sortedIndices.size() / 2;
    std::vector<uint32_t> leftIndices(sortedIndices.begin(), 
                                      sortedIndices.begin() + mid);
    std::vector<uint32_t> rightIndices(sortedIndices.begin() + mid, 
                                       sortedIndices.end());
    
    // 递归构建子树
    if (!leftIndices.empty())
    {
        node->m_left = BuildRecursive(leftIndices, depth + 1);
    }
    
    if (!rightIndices.empty())
    {
        node->m_right = BuildRecursive(rightIndices, depth + 1);
    }
    
    return node;
}

// ========================================
// 计算 Bounds
// ========================================

AABB3 CardBVH::ComputeCardBounds(uint32_t cardIndex) const
{
    if (!m_cards || cardIndex >= m_cards->size())
        return AABB3(Vec3(0, 0, 0), Vec3(0, 0, 0));
    
    const SurfaceCardMetadata& card = (*m_cards)[cardIndex];
    
    // 从 Card Metadata 重建世界空间的 AABB
    Vec3 origin(card.m_originX, card.m_originY, card.m_originZ);
    Vec3 axisX(card.m_axisXx, card.m_axisXy, card.m_axisXz);
    Vec3 axisY(card.m_axisYx, card.m_axisYy, card.m_axisYz);
    
    // Card 的 4 个角点
    Vec3 halfSizeX = axisX * (card.m_worldSizeX * 0.5f);
    Vec3 halfSizeY = axisY * (card.m_worldSizeY * 0.5f);
    
    Vec3 corners[4];
    corners[0] = origin - halfSizeX - halfSizeY;
    corners[1] = origin + halfSizeX - halfSizeY;
    corners[2] = origin - halfSizeX + halfSizeY;
    corners[3] = origin + halfSizeX + halfSizeY;
    
    // 计算包围盒
    AABB3 bounds;
    for (int i = 0; i < 4; i++)
    {
        bounds.StretchToIncludePoint(corners[i]);
    }
    
    return bounds;
}

AABB3 CardBVH::ComputeBounds(const std::vector<uint32_t>& cardIndices) const
{
    AABB3 bounds;
    
    for (uint32_t cardIndex : cardIndices)
    {
        AABB3 cardBounds = ComputeCardBounds(cardIndex);
        bounds.StretchToIncludePoint(cardBounds.m_mins);
        bounds.StretchToIncludePoint(cardBounds.m_maxs);
    }
    
    return bounds;
}

int CardBVH::ChooseSplitAxis(const std::vector<uint32_t>& cardIndices) const
{
    AABB3 bounds = ComputeBounds(cardIndices);
    Vec3 size = bounds.GetBoundsSize();
    
    // 选择最长的轴
    if (size.x >= size.y && size.x >= size.z)
        return 0;  // X 轴
    if (size.y >= size.z)
        return 1;  // Y 轴
    return 2;      // Z 轴
}

Vec3 CardBVH::GetCardCenter(uint32_t cardIndex) const
{
    if (!m_cards || cardIndex >= m_cards->size())
        return Vec3(0, 0, 0);
    
    const SurfaceCardMetadata& card = (*m_cards)[cardIndex];
    return Vec3(card.m_originX, card.m_originY, card.m_originZ);
}

// ========================================
// 查询：AABB 相交
// ========================================

void CardBVH::QueryIntersecting(const AABB3& bounds, std::vector<uint32_t>& outCardIndices) const
{
    outCardIndices.clear();
    
    if (!m_root)
        return;
    
    QueryRecursive(m_root.get(), bounds, outCardIndices);
}

void CardBVH::QueryRecursive(
    const CardBVHNode* node,
    const AABB3& bounds,
    std::vector<uint32_t>& outCardIndices) const
{
    if (!node)
        return;
    
    // AABB vs AABB 测试
    if (!DoAABBsOverlap3D(node->m_bounds, bounds))
        return;
    
    if (node->IsLeaf())
    {
        // 叶子节点：添加所有 Cards
        outCardIndices.insert(outCardIndices.end(),
                             node->m_cardIndices.begin(),
                             node->m_cardIndices.end());
        return;
    }
    
    // 递归查询子节点
    if (node->m_left)
        QueryRecursive(node->m_left.get(), bounds, outCardIndices);
    
    if (node->m_right)
        QueryRecursive(node->m_right.get(), bounds, outCardIndices);
}

// ========================================
// 查询：射线相交
// ========================================

void CardBVH::QueryRay(
    const Vec3& origin,
    const Vec3& dir,
    float maxDist,
    std::vector<uint32_t>& outCardIndices) const
{
    outCardIndices.clear();
    
    if (!m_root)
        return;
    
    QueryRayRecursive(m_root.get(), origin, dir, maxDist, outCardIndices);
}

void CardBVH::QueryRayRecursive(
    const CardBVHNode* node,
    const Vec3& origin,
    const Vec3& dir,
    float maxDist,
    std::vector<uint32_t>& outCardIndices) const
{
    if (!node)
        return;
    
    // 射线 vs AABB 测试
    if (!RaycastVsAABB3D(origin, dir, maxDist, node->m_bounds).m_didImpact)
        return;
    
    if (node->IsLeaf())
    {
        // 叶子节点：添加所有 Cards
        outCardIndices.insert(outCardIndices.end(),
                             node->m_cardIndices.begin(),
                             node->m_cardIndices.end());
        return;
    }
    
    // 递归查询子节点
    if (node->m_left)
        QueryRayRecursive(node->m_left.get(), origin, dir, maxDist, outCardIndices);
    
    if (node->m_right)
        QueryRayRecursive(node->m_right.get(), origin, dir, maxDist, outCardIndices);
}

// bool CardBVH::RayAABBIntersect(
//     const Vec3& origin,
//     const Vec3& dir,
//     const AABB3& bounds,
//     float maxDist) const
// {
//     // 射线 vs AABB 相交测试（Slab method）
//     Vec3 invDir(
//         fabsf(dir.x) > 0.0001f ? 1.0f / dir.x : 1e10f,
//         fabsf(dir.y) > 0.0001f ? 1.0f / dir.y : 1e10f,
//         fabsf(dir.z) > 0.0001f ? 1.0f / dir.z : 1e10f
//     );
//     
//     Vec3 t0 = (bounds.m_mins - origin).ComponentwiseMultiply(invDir);
//     Vec3 t1 = (bounds.m_maxs - origin).ComponentwiseMultiply(invDir);
//     
//     Vec3 tmin(MinF(t0.x, t1.x), MinF(t0.y, t1.y), MinF(t0.z, t1.z));
//     Vec3 tmax(MaxF(t0.x, t1.x), MaxF(t0.y, t1.y), MaxF(t0.z, t1.z));
//     
//     float tNear = MaxF(MaxF(tmin.x, tmin.y), tmin.z);
//     float tFar = MinF(MinF(tmax.x, tmax.y), tmax.z);
//     
//     return tNear <= tFar && tFar >= 0.0f && tNear <= maxDist;
// }

// ========================================
// 查询：点附近
// ========================================

void CardBVH::QueryNearby(
    const Vec3& point,
    float radius,
    std::vector<uint32_t>& outCardIndices) const
{
    outCardIndices.clear();
    
    if (!m_root)
        return;
    
    QueryNearbyRecursive(m_root.get(), point, radius, outCardIndices);
}

void CardBVH::QueryNearbyRecursive(
    const CardBVHNode* node,
    const Vec3& point,
    float radius,
    std::vector<uint32_t>& outCardIndices) const
{
    if (!node)
        return;
    
    // 扩展 AABB
    AABB3 expandedBounds = node->m_bounds;
    expandedBounds.m_mins -= Vec3(radius, radius, radius);
    expandedBounds.m_maxs += Vec3(radius, radius, radius);
    
    if (!expandedBounds.IsPointInside(point))
        return;
    
    if (node->IsLeaf())
    {
        // 叶子节点：添加所有 Cards
        outCardIndices.insert(outCardIndices.end(),
                             node->m_cardIndices.begin(),
                             node->m_cardIndices.end());
        return;
    }
    
    // 递归查询子节点
    if (node->m_left)
        QueryNearbyRecursive(node->m_left.get(), point, radius, outCardIndices);
    
    if (node->m_right)
        QueryNearbyRecursive(node->m_right.get(), point, radius, outCardIndices);
}

// ========================================
// GPU 扁平化
// ========================================

void CardBVH::FlattenForGPU(
    std::vector<GPUCardBVHNode>& outNodes,
    std::vector<uint32_t>& outCardIndices) const
{
    outNodes.clear();
    outCardIndices.clear();
    
    if (!m_root)
    {
        DebuggerPrintf("[CardBVH] Cannot flatten: root is null\n");
        return;
    }
    
    // 预分配空间
    outNodes.reserve(m_nodeCount);
    outCardIndices.reserve(m_cards ? m_cards->size() : 0);
    
    uint32_t nodeIndex = 0;
    FlattenRecursive(m_root.get(), outNodes, outCardIndices, nodeIndex);
    
    DebuggerPrintf("[CardBVH] Flattened: %zu nodes, %zu card indices\n",
                   outNodes.size(), outCardIndices.size());
}

void CardBVH::FlattenRecursive(
    const CardBVHNode* node,
    std::vector<GPUCardBVHNode>& outNodes,
    std::vector<uint32_t>& outCardIndices,
    uint32_t& nodeIndex) const
{
    if (!node)
        return;
    
    uint32_t currentIndex = nodeIndex++;
    
    // 为当前节点分配空间
    if (currentIndex >= outNodes.size())
        outNodes.resize(currentIndex + 1);
    
    GPUCardBVHNode& gpuNode = outNodes[currentIndex];
    
    // 设置 AABB
    gpuNode.m_boundsMinX = node->m_bounds.m_mins.x;
    gpuNode.m_boundsMinY = node->m_bounds.m_mins.y;
    gpuNode.m_boundsMinZ = node->m_bounds.m_mins.z;
    gpuNode.m_boundsMaxX = node->m_bounds.m_maxs.x;
    gpuNode.m_boundsMaxY = node->m_bounds.m_maxs.y;
    gpuNode.m_boundsMaxZ = node->m_bounds.m_maxs.z;
    
    if (node->IsLeaf())
    {
        // 叶子节点：记录 Card 信息
        gpuNode.m_leftFirst = static_cast<uint32_t>(outCardIndices.size());
        gpuNode.m_cardCount = static_cast<uint32_t>(node->m_cardIndices.size());
        
        // 添加 Card 索引
        for (uint32_t cardIdx : node->m_cardIndices)
        {
            outCardIndices.push_back(cardIdx);
        }
    }
    else
    {
        // 内部节点
        gpuNode.m_cardCount = 0;
        
        // 左孩子紧跟当前节点
        gpuNode.m_leftFirst = nodeIndex;
        
        // 递归扁平化子树
        if (node->m_left)
            FlattenRecursive(node->m_left.get(), outNodes, outCardIndices, nodeIndex);
        
        if (node->m_right)
            FlattenRecursive(node->m_right.get(), outNodes, outCardIndices, nodeIndex);
    }
}