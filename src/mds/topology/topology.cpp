/*
 * Project: curve
 * Created Date: Fri Aug 17 2018
 * Author: xuchaojie
 * Copyright (c) 2018 netease
 */
#include "src/mds/topology/topology.h"

#include <glog/logging.h>
#include <sys/time.h>

namespace curve {
namespace mds {
namespace topology {

PoolIdType TopologyImpl::AllocateLogicalPoolId() {
    return idGenerator_->GenLogicalPoolId();
}

PoolIdType TopologyImpl::AllocatePhysicalPoolId() {
    return idGenerator_->GenPhysicalPoolId();
}

ZoneIdType TopologyImpl::AllocateZoneId() {
    return idGenerator_->GenZoneId();
}

ServerIdType TopologyImpl::AllocateServerId() {
    return idGenerator_->GenServerId();
}

ChunkServerIdType TopologyImpl::AllocateChunkServerId() {
    return idGenerator_->GenChunkServerId();
}

std::string TopologyImpl::AllocateToken() {
    return tokenGenerator_->GenToken();
}

int TopologyImpl::AddLogicalPool(const LogicalPool &data) {
    std::lock_guard<curve::common::mutex> lockPhysicalPool(physicalPoolMutex_);
    std::lock_guard<curve::common::mutex> lockLogicalPool(logicalPoolMutex_);
    auto it = physicalPoolMap_.find(data.GetPhysicalPoolId());
    if (it != physicalPoolMap_.end()) {
        if (logicalPoolMap_.find(data.GetId()) == logicalPoolMap_.end()) {
            if (!storage_->StorageLogicalPool(data)) {
                return kTopoErrCodeStorgeFail;
            }
            logicalPoolMap_[data.GetId()] = data;
            return kTopoErrCodeSuccess;
        } else {
            return kTopoErrCodeIdDuplicated;
        }
    } else {
        return kTopoErrCodePhysicalPoolNotFound;
    }
}

int TopologyImpl::AddPhysicalPool(const PhysicalPool &data) {
    std::lock_guard<curve::common::mutex> lockPhysicalPool(physicalPoolMutex_);
    if (physicalPoolMap_.find(data.GetId()) == physicalPoolMap_.end()) {
        if (!storage_->StoragePhysicalPool(data)) {
            return kTopoErrCodeStorgeFail;
        }
        physicalPoolMap_[data.GetId()] = data;
        return kTopoErrCodeSuccess;
    } else {
        return kTopoErrCodeIdDuplicated;
    }
}

int TopologyImpl::AddZone(const Zone &data) {
    std::lock_guard<curve::common::mutex> lockPhysicalPool(physicalPoolMutex_);
    std::lock_guard<curve::common::mutex> lockZone(zoneMutex_);
    auto it = physicalPoolMap_.find(data.GetPhysicalPoolId());
    if (it != physicalPoolMap_.end()) {
        if (zoneMap_.find(data.GetId()) == zoneMap_.end()) {
            if (!storage_->StorageZone(data)) {
                return kTopoErrCodeStorgeFail;
            }
            it->second.AddZone(data.GetId());
            zoneMap_[data.GetId()] = data;
            return kTopoErrCodeSuccess;
        } else {
            return kTopoErrCodeIdDuplicated;
        }
    } else {
        return kTopoErrCodePhysicalPoolNotFound;
    }
}

int TopologyImpl::AddServer(const Server &data) {
    std::lock_guard<curve::common::mutex> lockZone(zoneMutex_);
    std::lock_guard<curve::common::mutex> lockServer(serverMutex_);
    auto it = zoneMap_.find(data.GetZoneId());
    if (it != zoneMap_.end()) {
        if (serverMap_.find(data.GetId()) == serverMap_.end()) {
            if (!storage_->StorageServer(data)) {
                return kTopoErrCodeStorgeFail;
            }
            it->second.AddServer(data.GetId());
            serverMap_[data.GetId()] = data;
            return kTopoErrCodeSuccess;
        } else {
            return kTopoErrCodeIdDuplicated;
        }
    } else {
        return kTopoErrCodeZoneNotFound;
    }
}

int TopologyImpl::AddChunkServer(const ChunkServer &data) {
    std::lock_guard<curve::common::mutex> lockServer(serverMutex_);
    std::lock_guard<curve::common::mutex> lockChunkServer(chunkServerMutex_);
    auto it = serverMap_.find(data.GetServerId());
    if (it != serverMap_.end()) {
        if (chunkServerMap_.find(data.GetId()) == chunkServerMap_.end()) {
            if (!storage_->StorageChunkServer(data)) {
                return kTopoErrCodeStorgeFail;
            }
            it->second.AddChunkServer(data.GetId());
            chunkServerMap_[data.GetId()] = data;
            return kTopoErrCodeSuccess;
        } else {
            return kTopoErrCodeIdDuplicated;
        }
    } else {
        return kTopoErrCodeServerNotFound;
    }
}

int TopologyImpl::RemoveLogicalPool(PoolIdType id) {
    std::lock_guard<curve::common::mutex> lockLogicalPool(logicalPoolMutex_);
    auto it = logicalPoolMap_.find(id);
    if (it != logicalPoolMap_.end()) {
        if (!storage_->DeleteLogicalPool(id)) {
            return kTopoErrCodeStorgeFail;
        }
        logicalPoolMap_.erase(it);
        return kTopoErrCodeSuccess;
    } else {
        return kTopoErrCodeLogicalPoolNotFound;
    }
}

int TopologyImpl::RemovePhysicalPool(PoolIdType id) {
    std::lock_guard<curve::common::mutex> lockPhysicalPool(physicalPoolMutex_);
    auto it = physicalPoolMap_.find(id);
    if (it != physicalPoolMap_.end()) {
        if (!storage_->DeletePhysicalPool(id)) {
            return kTopoErrCodeStorgeFail;
        }
        physicalPoolMap_.erase(it);
        return kTopoErrCodeSuccess;
    } else {
        return kTopoErrCodePhysicalPoolNotFound;
    }
}

int TopologyImpl::RemoveZone(ZoneIdType id) {
    std::lock_guard<curve::common::mutex> lockPhysicalPool(physicalPoolMutex_);
    std::lock_guard<curve::common::mutex> lockZone(zoneMutex_);
    auto it = zoneMap_.find(id);
    if (it != zoneMap_.end()) {
        if (!storage_->DeleteZone(id)) {
            return kTopoErrCodeStorgeFail;
        }
        auto ix = physicalPoolMap_.find(it->second.GetPhysicalPoolId());
        if (ix != physicalPoolMap_.end()) {
            ix->second.RemoveZone(id);
        }
        zoneMap_.erase(it);
        return kTopoErrCodeSuccess;
    } else {
        return kTopoErrCodeZoneNotFound;
    }
}

int TopologyImpl::RemoveServer(ServerIdType id) {
    std::lock_guard<curve::common::mutex> lockZone(zoneMutex_);
    std::lock_guard<curve::common::mutex> lockServer(serverMutex_);
    auto it = serverMap_.find(id);
    if (it != serverMap_.end()) {
        if (!storage_->DeleteServer(id)) {
            return kTopoErrCodeStorgeFail;
        }
        auto ix = zoneMap_.find(it->second.GetZoneId());
        if (ix != zoneMap_.end()) {
            ix->second.RemoveServer(id);
        }
        serverMap_.erase(it);
        return kTopoErrCodeSuccess;
    } else {
        return kTopoErrCodeServerNotFound;
    }
}

int TopologyImpl::RemoveChunkServer(ChunkServerIdType id) {
    std::lock_guard<curve::common::mutex> lockServer(serverMutex_);
    std::lock_guard<curve::common::mutex> lockChunkServer(chunkServerMutex_);
    auto it = chunkServerMap_.find(id);
    if (it != chunkServerMap_.end()) {
        if (!storage_->DeleteChunkServer(id)) {
            return kTopoErrCodeStorgeFail;
        }
        auto ix = serverMap_.find(it->second.GetServerId());
        if (ix != serverMap_.end()) {
            ix->second.RemoveChunkServer(id);
        }
        chunkServerMap_.erase(it);
        return kTopoErrCodeSuccess;
    } else {
        return kTopoErrCodeChunkServerNotFound;
    }
}

int TopologyImpl::UpdateLogicalPool(const LogicalPool &data) {
    std::lock_guard<curve::common::mutex> lockLogicalPool(logicalPoolMutex_);
    auto it = logicalPoolMap_.find(data.GetId());
    if (it != logicalPoolMap_.end()) {
        if (!storage_->UpdateLogicalPool(data)) {
            return kTopoErrCodeStorgeFail;
        }
        it->second = data;
        return kTopoErrCodeSuccess;
    } else {
        return kTopoErrCodeLogicalPoolNotFound;
    }
}

int TopologyImpl::UpdatePhysicalPool(const PhysicalPool &data) {
    std::lock_guard<curve::common::mutex> lockPhysicalPool(physicalPoolMutex_);
    auto it = physicalPoolMap_.find(data.GetId());
    if (it != physicalPoolMap_.end()) {
        if (!storage_->UpdatePhysicalPool(data)) {
            return kTopoErrCodeStorgeFail;
        }
        it->second = data;
        return kTopoErrCodeSuccess;
    } else {
        return kTopoErrCodePhysicalPoolNotFound;
    }
}

int TopologyImpl::UpdateZone(const Zone &data) {
    std::lock_guard<curve::common::mutex> lockZone(zoneMutex_);
    auto it = zoneMap_.find(data.GetId());
    if (it != zoneMap_.end()) {
        if (!storage_->UpdateZone(data)) {
            return kTopoErrCodeStorgeFail;
        }
        it->second = data;
        return kTopoErrCodeSuccess;
    } else {
        return kTopoErrCodeZoneNotFound;
    }
}

int TopologyImpl::UpdateServer(const Server &data) {
    std::lock_guard<curve::common::mutex> lockServer(serverMutex_);
    auto it = serverMap_.find(data.GetId());
    if (it != serverMap_.end()) {
        if (!storage_->UpdateServer(data)) {
            return kTopoErrCodeStorgeFail;
        }
        it->second = data;
        return kTopoErrCodeSuccess;
    } else {
        return kTopoErrCodeServerNotFound;
    }
}

// 更新内存并持久化全部数据
int TopologyImpl::UpdateChunkServer(const ChunkServer &data) {
    std::lock_guard<curve::common::mutex> lockChunkServer(chunkServerMutex_);
    auto it = chunkServerMap_.find(data.GetId());
    if (it != chunkServerMap_.end()) {
        if (!storage_->UpdateChunkServer(data)) {
            return kTopoErrCodeStorgeFail;
        }
        it->second = data;
        return kTopoErrCodeSuccess;
    } else {
        return kTopoErrCodeChunkServerNotFound;
    }
}

// 更新内存，定期持久化数据
int TopologyImpl::UpdateChunkServerState(const ChunkServerState &state,
                                         ChunkServerIdType id) {
    std::lock_guard<curve::common::mutex> lockChunkServer(chunkServerMutex_);
    auto it = chunkServerMap_.find(id);
    if (it != chunkServerMap_.end()) {
        timeval now;
        gettimeofday(&now, NULL);
        uint64_t currentTime = now.tv_sec;
        uint64_t lastTime = it->second.GetLastStateUpdateTime();
        if ((currentTime - lastTime) >= kChunkServerStateUpdateFreq) {
            ChunkServer cs = it->second;
            cs.SetChunkServerState(state);
            cs.SetLastStateUpdateTime(currentTime);
            if (!storage_->UpdateChunkServer(cs)) {
                return kTopoErrCodeStorgeFail;
            }
            it->second = cs;
        }
        return kTopoErrCodeSuccess;
    } else {
        return kTopoErrCodeChunkServerNotFound;
    }
}

PoolIdType TopologyImpl::FindLogicalPool(
    const std::string &logicalPoolName,
    const std::string &physicalPoolName) const {
    PoolIdType physicalPoolId = FindPhysicalPool(physicalPoolName);
    std::lock_guard<curve::common::mutex> lockLogicalPool(logicalPoolMutex_);
    for (auto it = logicalPoolMap_.begin();
         it != logicalPoolMap_.end();
         it++) {
        if ((it->second.GetPhysicalPoolId() == physicalPoolId) &&
            (it->second.GetName() == logicalPoolName)) {
            return it->first;
        }
    }
    return static_cast<PoolIdType>(UNINTIALIZE_ID);
}

PoolIdType TopologyImpl::FindPhysicalPool(
    const std::string &physicalPoolName) const {
    std::lock_guard<curve::common::mutex> lockPhysicalPool(physicalPoolMutex_);
    for (auto it = physicalPoolMap_.begin();
         it != physicalPoolMap_.end();
         it++) {
        if (it->second.GetName() == physicalPoolName) {
            return it->first;
        }
    }
    return static_cast<PoolIdType>(UNINTIALIZE_ID);
}

ZoneIdType TopologyImpl::FindZone(const std::string &zoneName,
                                  const std::string &physicalPoolName) const {
    PoolIdType physicalPoolId = FindPhysicalPool(physicalPoolName);
    return FindZone(zoneName, physicalPoolId);
}

ZoneIdType TopologyImpl::FindZone(const std::string &zoneName,
                                  PoolIdType physicalPoolId) const {
    std::lock_guard<curve::common::mutex> lockZone(zoneMutex_);
    for (auto it = zoneMap_.begin(); it != zoneMap_.end(); it++) {
        if ((it->second.GetPhysicalPoolId() == physicalPoolId) &&
            (it->second.GetName() == zoneName)) {
            return it->first;
        }
    }
    return static_cast<ZoneIdType>(UNINTIALIZE_ID);
}

ServerIdType TopologyImpl::FindServerByHostName(
    const std::string &hostName) const {
    std::lock_guard<curve::common::mutex> lockServer(serverMutex_);
    for (auto it = serverMap_.begin(); it != serverMap_.end(); it++) {
        if (it->second.GetHostName() == hostName) {
            return it->first;
        }
    }
    return static_cast<ServerIdType>(UNINTIALIZE_ID);
}

ServerIdType TopologyImpl::FindServerByHostIp(const std::string &hostIp) const {
    std::lock_guard<curve::common::mutex> lockServer(serverMutex_);
    for (auto it = serverMap_.begin(); it != serverMap_.end(); it++) {
        if ((it->second.GetInternalHostIp() == hostIp) ||
            (it->second.GetExternalHostIp() == hostIp)) {
            return it->first;
        }
    }
    return static_cast<ServerIdType>(UNINTIALIZE_ID);
}

ChunkServerIdType TopologyImpl::FindChunkServer(const std::string &hostIp,
                                                uint32_t port) const {
    ServerIdType serverId = FindServerByHostIp(hostIp);
    std::lock_guard<curve::common::mutex> lockChunkServer(chunkServerMutex_);
    for (auto it = chunkServerMap_.begin();
         it != chunkServerMap_.end();
         it++) {
        if ((it->second.GetServerId() == serverId) &&
            (it->second.GetPort() == port)) {
            return it->first;
        }
    }
    return static_cast<ChunkServerIdType>(UNINTIALIZE_ID);
}

bool TopologyImpl::GetLogicalPool(PoolIdType poolId, LogicalPool *out) const {
    std::lock_guard<curve::common::mutex> lockLogicalPool(logicalPoolMutex_);
    auto it = logicalPoolMap_.find(poolId);
    if (it != logicalPoolMap_.end()) {
        *out = it->second;
        return true;
    }
    return false;
}

bool TopologyImpl::GetPhysicalPool(PoolIdType poolId, PhysicalPool *out) const {
    std::lock_guard<curve::common::mutex> lockPhysicalPool(physicalPoolMutex_);
    auto it = physicalPoolMap_.find(poolId);
    if (it != physicalPoolMap_.end()) {
        *out = it->second;
        return true;
    }
    return false;
}

bool TopologyImpl::GetZone(ZoneIdType zoneId, Zone *out) const {
    std::lock_guard<curve::common::mutex> lockZone(zoneMutex_);
    auto it = zoneMap_.find(zoneId);
    if (it != zoneMap_.end()) {
        *out = it->second;
        return true;
    }
    return false;
}

bool TopologyImpl::GetServer(ServerIdType serverId, Server *out) const {
    std::lock_guard<curve::common::mutex> lockServer(serverMutex_);
    auto it = serverMap_.find(serverId);
    if (it != serverMap_.end()) {
        *out = it->second;
        return true;
    }
    return false;
}

bool TopologyImpl::GetChunkServer(ChunkServerIdType chunkserverId,
                                  ChunkServer *out) const {
    std::lock_guard<curve::common::mutex> lockChunkServer(chunkServerMutex_);
    auto it = chunkServerMap_.find(chunkserverId);
    if (it != chunkServerMap_.end()) {
        *out = it->second;
        return true;
    }
    return false;
}


////////////////////////////////////////////////////////////////////////////////
// getList

std::list<ChunkServerIdType> TopologyImpl::GetChunkServerInCluster() const {
    std::list<ChunkServerIdType> ret;
    std::lock_guard<curve::common::mutex> lockChunkServer(chunkServerMutex_);
    for (auto it = chunkServerMap_.begin();
         it != chunkServerMap_.end();
         it++) {
        ret.push_back(it->first);
    }
    return ret;
}

std::list<ServerIdType> TopologyImpl::GetServerInCluster() const {
    std::list<ServerIdType> ret;
    std::lock_guard<curve::common::mutex> lockServer(serverMutex_);
    for (auto it = serverMap_.begin(); it != serverMap_.end(); it++) {
        ret.push_back(it->first);
    }
    return ret;
}

std::list<ZoneIdType> TopologyImpl::GetZoneInCluster() const {
    std::list<ZoneIdType> ret;
    std::lock_guard<curve::common::mutex> lockZone(zoneMutex_);
    for (auto it = zoneMap_.begin(); it != zoneMap_.end(); it++) {
        ret.push_back(it->first);
    }
    return ret;
}

std::list<PoolIdType> TopologyImpl::GetPhysicalPoolInCluster() const {
    std::list<PoolIdType> ret;
    std::lock_guard<curve::common::mutex> lockPhysicalPool(physicalPoolMutex_);
    for (auto it = physicalPoolMap_.begin();
         it != physicalPoolMap_.end();
         it++) {
        ret.push_back(it->first);
    }
    return ret;
}

std::list<PoolIdType> TopologyImpl::GetLogicalPoolInCluster() const {
    std::list<PoolIdType> ret;
    std::lock_guard<curve::common::mutex> lockLogicalPool(logicalPoolMutex_);
    for (auto it = logicalPoolMap_.begin();
         it != logicalPoolMap_.end();
         it++) {
        ret.push_back(it->first);
    }
    return ret;
}

std::list<ChunkServerIdType> TopologyImpl::GetChunkServerInServer(
    ServerIdType id) const {
    Server server;
    if (GetServer(id, &server)) {
        return server.GetChunkServerList();
    }
    return std::list<ChunkServerIdType>();
}

std::list<ChunkServerIdType> TopologyImpl::GetChunkServerInZone(
    ZoneIdType id) const {
    std::list<ChunkServerIdType> ret;
    std::list<ServerIdType> serverList = GetServerInZone(id);
    for (ServerIdType s : serverList) {
        std::list<ChunkServerIdType> temp = GetChunkServerInServer(s);
        ret.splice(ret.begin(), temp);
    }
    return ret;
}

std::list<ChunkServerIdType> TopologyImpl::GetChunkServerInPhysicalPool(
    PoolIdType id) const {
    std::list<ChunkServerIdType> ret;
    std::list<ServerIdType> serverList = GetServerInPhysicalPool(id);
    for (ServerIdType s : serverList) {
        std::list<ChunkServerIdType> temp = GetChunkServerInServer(s);
        ret.splice(ret.begin(), temp);
    }
    return ret;
}

std::list<ServerIdType> TopologyImpl::GetServerInZone(ZoneIdType id) const {
    Zone zone;
    if (GetZone(id, &zone)) {
        return zone.GetServerList();
    }
    return std::list<ServerIdType>();
}

std::list<ServerIdType> TopologyImpl::GetServerInPhysicalPool(
    PoolIdType id) const {
    std::list<ServerIdType> ret;
    std::list<ZoneIdType> zoneList = GetZoneInPhysicalPool(id);
    for (ZoneIdType z : zoneList) {
        std::list<ServerIdType> temp = GetServerInZone(z);
        ret.splice(ret.begin(), temp);
    }
    return ret;
}

std::list<ZoneIdType> TopologyImpl::GetZoneInPhysicalPool(PoolIdType id) const {
    PhysicalPool pool;
    if (GetPhysicalPool(id, &pool)) {
        return pool.GetZoneList();
    }
    return std::list<ZoneIdType>();
}

std::list<PoolIdType> TopologyImpl::GetLogicalPoolInPhysicalPool(
    PoolIdType id) const {
    std::list<PoolIdType> ret;
    std::lock_guard<curve::common::mutex> lockLogicalPool(logicalPoolMutex_);
    for (auto it = logicalPoolMap_.begin(); it != logicalPoolMap_.end(); it++) {
        if (it->second.GetPhysicalPoolId() == id) {
            ret.push_back(it->first);
        }
    }
    return ret;
}

std::list<ChunkServerIdType> TopologyImpl::GetChunkServerInLogicalPool(
    PoolIdType id) const {
    LogicalPool lPool;
    if (GetLogicalPool(id, &lPool)) {
        return GetChunkServerInPhysicalPool(lPool.GetPhysicalPoolId());
    }
    return std::list<ChunkServerIdType>();
}

std::list<ServerIdType> TopologyImpl::GetServerInLogicalPool(
    PoolIdType id) const {
    LogicalPool lPool;
    if (GetLogicalPool(id, &lPool)) {
        return GetServerInPhysicalPool(lPool.GetPhysicalPoolId());
    }
    return std::list<ServerIdType>();
}

std::list<ZoneIdType> TopologyImpl::GetZoneInLogicalPool(PoolIdType id) const {
    LogicalPool lPool;
    if (GetLogicalPool(id, &lPool)) {
        return GetZoneInPhysicalPool(lPool.GetPhysicalPoolId());
    }
    return std::list<ZoneIdType>();
}

int TopologyImpl::init() {
    std::lock_guard<curve::common::mutex> lockLogicalPool(logicalPoolMutex_);
    std::lock_guard<curve::common::mutex> lockPhysicalPool(physicalPoolMutex_);
    std::lock_guard<curve::common::mutex> lockZone(zoneMutex_);
    std::lock_guard<curve::common::mutex> lockServer(serverMutex_);
    std::lock_guard<curve::common::mutex> lockChunkServer(chunkServerMutex_);
    std::lock_guard<curve::common::mutex> lockCopySet(copySetMutex_);

    PoolIdType maxLogicalPoolId;
    if (!storage_->LoadLogicalPool(&logicalPoolMap_, &maxLogicalPoolId)) {
        LOG(ERROR) << "[TopologyImpl::init], LoadLogicalPool fail.";
        return kTopoErrCodeStorgeFail;
    }
    idGenerator_->initLogicalPoolIdGenerator(maxLogicalPoolId);

    PoolIdType maxPhysicalPoolId;
    if (!storage_->LoadPhysicalPool(&physicalPoolMap_, &maxPhysicalPoolId)) {
        LOG(ERROR) << "[TopologyImpl::init], LoadPhysicalPool fail.";
        return kTopoErrCodeStorgeFail;
    }
    idGenerator_->initPhysicalPoolIdGenerator(maxPhysicalPoolId);

    ZoneIdType maxZoneId;
    if (!storage_->LoadZone(&zoneMap_, &maxZoneId)) {
        LOG(ERROR) << "[TopologyImpl::init], LoadZone fail.";
        return kTopoErrCodeStorgeFail;
    }
    idGenerator_->initZoneIdGenerator(maxZoneId);

    ServerIdType maxServerId;
    if (!storage_->LoadServer(&serverMap_, &maxServerId)) {
        LOG(ERROR) << "[TopologyImpl::init], LoadServer fail.";
        return kTopoErrCodeStorgeFail;
    }
    idGenerator_->initServerIdGenerator(maxServerId);

    ChunkServerIdType maxChunkServerId;
    if (!storage_->LoadChunkServer(&chunkServerMap_, &maxChunkServerId)) {
        LOG(ERROR) << "[TopologyImpl::init], LoadChunkServer fail.";
        return kTopoErrCodeStorgeFail;
    }
    idGenerator_->initChunkServerIdGenerator(maxChunkServerId);

    std::map<PoolIdType, CopySetIdType> copySetIdMaxMap;
    if (!storage_->LoadCopySet(&copySetMap_, &copySetIdMaxMap)) {
        LOG(ERROR) << "[TopologyImpl::init], LoadCopySet fail.";
        return kTopoErrCodeStorgeFail;
    }
    idGenerator_->initCopySetIdGenerator(copySetIdMaxMap);

    for (auto it : zoneMap_) {
        PoolIdType poolid = it.second.GetPhysicalPoolId();
        physicalPoolMap_[poolid].AddZone(it.first);
    }

    for (auto it : serverMap_) {
        ZoneIdType zid = it.second.GetZoneId();
        zoneMap_[zid].AddServer(it.first);
    }

    for (auto it : chunkServerMap_) {
        ServerIdType sId = it.second.GetServerId();
        serverMap_[sId].AddChunkServer(it.first);
    }

    return kTopoErrCodeSuccess;
}

CopySetIdType TopologyImpl::AllocateCopySetId(PoolIdType logicalPoolId) {
    return idGenerator_->GenCopySetId(logicalPoolId);
}

int TopologyImpl::AddCopySet(const CopySetInfo &data) {
    std::lock_guard<curve::common::mutex> lockLogicalPool(logicalPoolMutex_);
    std::lock_guard<curve::common::mutex> lockCopySet(copySetMutex_);
    auto it = logicalPoolMap_.find(data.GetLogicalPoolId());
    if (it != logicalPoolMap_.end()) {
        CopySetKey key(data.GetLogicalPoolId(), data.GetId());
        if (copySetMap_.find(key) == copySetMap_.end()) {
            if (!storage_->StorageCopySet(data)) {
                return kTopoErrCodeStorgeFail;
            }
            copySetMap_[key] = data;
            return kTopoErrCodeSuccess;
        } else {
            return kTopoErrCodeIdDuplicated;
        }
    } else {
        return kTopoErrCodeLogicalPoolNotFound;
    }
}

// TODO(xuchaojie): 优化该逻辑，移除下述事物操作
int TopologyImpl::AddCopySetList(const std::vector<CopySetInfo> &copysets) {
    // 获取所有的锁，数据库事务操作时不允许其他数据库操作
    std::lock_guard<curve::common::mutex> lockLogicalPool(logicalPoolMutex_);
    std::lock_guard<curve::common::mutex> lockPhysicalPool(physicalPoolMutex_);
    std::lock_guard<curve::common::mutex> lockZone(zoneMutex_);
    std::lock_guard<curve::common::mutex> lockServer(serverMutex_);
    std::lock_guard<curve::common::mutex> lockChunkServer(chunkServerMutex_);
    std::lock_guard<curve::common::mutex> lockCopySet(copySetMutex_);
    for (const CopySetInfo &data : copysets) {
        auto it = logicalPoolMap_.find(data.GetLogicalPoolId());
        if (it != logicalPoolMap_.end()) {
            CopySetKey key(data.GetLogicalPoolId(), data.GetId());
            if (copySetMap_.find(key) != copySetMap_.end()) {
                return kTopoErrCodeIdDuplicated;
            }
        } else {
            return kTopoErrCodeLogicalPoolNotFound;
        }
    }

    bool success = true;
    storage_->SetAutoCommit(false);
    for (const CopySetInfo &data : copysets) {
        if (!storage_->StorageCopySet(data)) {
            success = false;
            break;
        }
    }

    if (success) {
        storage_->Commit();
        storage_->SetAutoCommit(true);
        for (const CopySetInfo &data : copysets) {
            CopySetKey key(data.GetLogicalPoolId(), data.GetId());
            copySetMap_[key] = data;
        }
        return kTopoErrCodeSuccess;
    } else {
        storage_->RollBack();
        storage_->SetAutoCommit(true);
        return kTopoErrCodeStorgeFail;
    }
}

int TopologyImpl::RemoveCopySet(CopySetKey key) {
    std::lock_guard<curve::common::mutex> lockCopySet(copySetMutex_);
    auto it = copySetMap_.find(key);
    if (it != copySetMap_.end()) {
        if (!storage_->DeleteCopySet(key)) {
            return kTopoErrCodeStorgeFail;
        }
        copySetMap_.erase(key);
        return kTopoErrCodeSuccess;
    } else {
        return kTopoErrCodeCopySetNotFound;
    }
}

int TopologyImpl::UpdateCopySet(const CopySetInfo &data) {
    std::lock_guard<curve::common::mutex> lockCopySet(copySetMutex_);
    CopySetKey key(data.GetLogicalPoolId(), data.GetId());
    auto it = copySetMap_.find(key);
    if (it != copySetMap_.end()) {
        if (!storage_->UpdateCopySet(data)) {
            return kTopoErrCodeStorgeFail;
        }
        it->second = data;
        return kTopoErrCodeSuccess;
    } else {
        return kTopoErrCodeCopySetNotFound;
    }
}

bool TopologyImpl::GetCopySet(CopySetKey key, CopySetInfo *out) const {
    std::lock_guard<curve::common::mutex> lockCopySet(copySetMutex_);
    auto it = copySetMap_.find(key);
    if (it != copySetMap_.end()) {
        *out = it->second;
        return true;
    } else {
        return false;
    }
}

std::vector<CopySetIdType> TopologyImpl::GetCopySetsInLogicalPool(
    PoolIdType logicalPoolId) const {
    std::vector<CopySetIdType> ret;
    std::lock_guard<curve::common::mutex> lockCopySet(copySetMutex_);
    for (auto it : copySetMap_) {
        if (it.first.first == logicalPoolId) {
            ret.push_back(it.first.second);
        }
    }
    return ret;
}
std::vector<CopySetKey> TopologyImpl::GetCopySetsInCluster() const {}
std::vector<CopySetKey> TopologyImpl::
GetCopySetsInChunkServer(ChunkServerIdType id) const {}
}  // namespace topology
}  // namespace mds
}  // namespace curve
