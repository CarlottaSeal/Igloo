// #include "RenderCommon.h"
//
// constexpr int PrimaryAtlasSrvIndex(int bufferIdx)
// {
// 	// bufferIdx: 0 or 1
// 	return SURFCACHE_PRIMARY_BASE + bufferIdx * DESCRIPTORS_PER_CACHE_BUFFER + 0;
// }
//
// constexpr int PrimaryMetaSrvIndex(int bufferIdx)
// {
// 	return SURFCACHE_PRIMARY_BASE + bufferIdx * DESCRIPTORS_PER_CACHE_BUFFER + 1;
// }
//
// constexpr int PrimaryRadianceUavIndex(int bufferIdx)
// {
// 	return SURFCACHE_PRIMARY_BASE + bufferIdx * DESCRIPTORS_PER_CACHE_BUFFER + 2;
// }
//
// constexpr int PrimaryMetaUavIndex(int bufferIdx)
// {
// 	return SURFCACHE_PRIMARY_BASE + bufferIdx * DESCRIPTORS_PER_CACHE_BUFFER + 3;
// }
//
// constexpr int GIAtlasSrvIndex(int bufferIdx)
// {
// 	return SURFCACHE_GI_BASE + bufferIdx * DESCRIPTORS_PER_CACHE_BUFFER + 0;
// }
//
// constexpr int GIMetaSrvIndex(int bufferIdx)
// {
// 	return SURFCACHE_GI_BASE + bufferIdx * DESCRIPTORS_PER_CACHE_BUFFER + 1;
// }
//
// constexpr int GIRadianceUavIndex(int bufferIdx)
// {
// 	return SURFCACHE_GI_BASE + bufferIdx * DESCRIPTORS_PER_CACHE_BUFFER + 2;
// }
//
// constexpr int GIMetaUavIndex(int bufferIdx)
// {
// 	return SURFCACHE_GI_BASE + bufferIdx * DESCRIPTORS_PER_CACHE_BUFFER + 3;
// }