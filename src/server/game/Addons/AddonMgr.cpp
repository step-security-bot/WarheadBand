/*
 * This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "AddonMgr.h"
#include "CryptoHash.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "StopWatch.h"
#include <list>

namespace AddonMgr
{
    // Anonymous namespace ensures file scope of all the stuff inside it, even
    // if you add something more to this namespace somewhere else.
    namespace
    {
        // List of saved addons (in DB).
        typedef std::list<SavedAddon> SavedAddonsList;

        SavedAddonsList m_knownAddons;
        BannedAddonList m_bannedAddons;
    }

    void LoadFromDB()
    {
        StopWatch sw;

        QueryResult result = CharacterDatabase.Query("SELECT name, crc FROM addons");
        if (!result)
        {
            LOG_WARN("server.loading", ">> Loaded 0 known addons. DB table `addons` is empty!");
            LOG_INFO("server.loading", " ");
            return;
        }

        uint32 count = 0;

        for (auto const& row : *result)
        {
            std::string name = row[0].Get<std::string>();
            uint32 crc = row[1].Get<uint32>();

            m_knownAddons.push_back(SavedAddon(name, crc));
            ++count;
        }

        LOG_INFO("server.loading", ">> Loaded {} known addons in {}", count, sw);
        LOG_INFO("server.loading", "");

        sw.Reset();
        result = CharacterDatabase.Query("SELECT id, name, version, UNIX_TIMESTAMP(timestamp) FROM banned_addons");

        if (result)
        {
            uint32 count2 = 0;
            uint32 offset = 102;

            for (auto const& fields : *result)
            {
                BannedAddon addon{};
                addon.Id = fields[0].Get<uint32>() + offset;
                addon.Timestamp = uint32(fields[3].Get<uint64>());
                addon.NameMD5 = Warhead::Crypto::MD5::GetDigestOf(fields[1].Get<std::string>());
                addon.VersionMD5 = Warhead::Crypto::MD5::GetDigestOf(fields[2].Get<std::string>());

                m_bannedAddons.emplace_back(addon);

                ++count2;
            } while (result->NextRow());

            LOG_INFO("server.loading", ">> Loaded {} banned addons in {}", count2, sw);
            LOG_INFO("server.loading", "");
        }
    }

    void SaveAddon(AddonInfo const& addon)
    {
        std::string name = addon.Name;

        CharacterDatabasePreparedStatement stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_ADDON);

        stmt->SetData(0, name);
        stmt->SetData(1, addon.CRC);

        CharacterDatabase.Execute(stmt);

        m_knownAddons.push_back(SavedAddon(addon.Name, addon.CRC));
    }

    SavedAddon const* GetAddonInfo(const std::string& name)
    {
        for (auto const& addon : m_knownAddons)
        {
            if (addon.Name == name)
            {
                return &addon;
            }
        }

        return nullptr;
    }

    BannedAddonList const* GetBannedAddons()
    {
        return &m_bannedAddons;
    }

} // Namespace
