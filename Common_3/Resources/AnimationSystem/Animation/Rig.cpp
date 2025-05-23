/*
 * Copyright (c) 2017-2025 The Forge Interactive Inc.
 *
 * This file is part of The-Forge
 * (see https://github.com/ConfettiFX/The-Forge).
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "Rig.h"

void Rig::Initialize(const ResourceDirectory resourceDir, const char* fileName)
{
    // Reading skeleton.
    if (!LoadSkeleton(resourceDir, fileName))
    {
        LOGF(eERROR, "Couldn't load skeleton %s", fileName);
        return; // need error catching
    }

    mNumSoaJoints = mSkeleton.num_soa_joints();
    mNumJoints = mSkeleton.num_joints();

    if (mNumJoints == 0)
    {
        LOGF(eERROR, "Couldn't load skeleton %s", fileName);
        return;
    }

    // Find the root index
    for (uint32_t i = 0; i < mNumJoints; i++)
    {
        if (mSkeleton.joint_parents()[i] == ozz::animation::Skeleton::kNoParent)
        {
            mRootIndex = i;
            break;
        }
    }
}

void Rig::Exit() { mSkeleton.Deallocate(); }

bool Rig::LoadSkeleton(const ResourceDirectory resourceDir, const char* fileName)
{
    FileStream file = {};
    if (!fsOpenStreamFromPath(resourceDir, fileName, FM_READ, &file))
    {
        LOGF(eERROR, "Cannot open skeleton file %s. Function %s failed with error: %s", fileName, FS_ERR_CTX.func,
             getFSErrCodeString(FS_ERR_CTX.code));
        return false;
    }

    ssize_t size = fsGetStreamFileSize(&file);
    void*   data = tf_malloc(size);
    fsReadFromStream(&file, data, (size_t)size);
    fsCloseStream(&file);

    // Archive is doing a lot of freads from disk which is slow on some platforms and also generally not good
    // So we just read the entire file once into a mem stream so the freads from IArchive are actually
    // only reading from system memory instead of disk or network
    FileStream memStream = {};
    fsOpenStreamFromMemory(data, size, FM_READ, true, &memStream);

    ozz::io::IArchive archive(&memStream);
    if (!archive.TestTag<ozz::animation::Skeleton>())
    {
        LOGF(eERROR, "Skeleton Archive doesn't contain the expected object type");
        fsCloseStream(&memStream);
        return false;
    }

    archive >> mSkeleton;

    fsCloseStream(&memStream);

    return true;
}

int32_t Rig::FindJoint(const char* jointName)
{
    for (uint32_t i = 0; i < mNumJoints; i++)
    {
        if (strcmp(mSkeleton.joint_names()[i], jointName) == 0)
            return i;
    }
    return -1;
}

void Rig::FindJointChain(const char* jointNames[], size_t numNames, int32_t jointChain[])
{
    size_t found = 0;
    for (int32_t i = 0; i < mSkeleton.num_joints() && found < numNames; ++i)
    {
        const char* joint_name = mSkeleton.joint_names()[i];
        if (strcmp(joint_name, jointNames[found]) == 0)
        {
            jointChain[found] = i;
            ++found;
            i = 0;
        }
    }
}
