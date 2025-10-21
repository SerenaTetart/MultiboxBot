#include "DBCReader.h"

const FactionTemplateEntry* FactionTemplateDBC::Get(uint32_t id) const {
    auto it = byId.find(id);
    return (it == byId.end() ? nullptr : &it->second);
}

bool FactionTemplateDBC::isNeutral(uint32_t ID) {
    if (const FactionTemplateEntry* e = Get(ID)) {
        if (e->EnemyGroup == 0) return true;
        else return false;
    }
    else return false;
}

uint32_t FactionTemplateDBC::readU32(const char* p) {
    // DBC = little-endian; si ta plateforme est little, on peut memcpy direct
    uint32_t v;
    std::memcpy(&v, p, sizeof(v));
    return v;
}

void FactionTemplateDBC::load(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Impossible d'ouvrir " + path);

    DbcHeader hdr{};
    f.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
    if (!f) throw std::runtime_error("Lecture header échouée: " + path);

    if (std::memcmp(hdr.magic, "WDBC", 4) != 0)
        throw std::runtime_error("Format non supporté (attendu WDBC): " + path);

    if (hdr.fieldCount < 14)
        throw std::runtime_error("FactionTemplate.dbc: trop peu de colonnes (attendu >=14)");

    if (hdr.recordSize < hdr.fieldCount * 4)
        throw std::runtime_error("recordSize invalide pour " + path);

    // Lis le bloc de records
    const size_t recordsBytes = static_cast<size_t>(hdr.recordCount) * hdr.recordSize;
    std::vector<char> records(recordsBytes);
    f.read(records.data(), recordsBytes);
    if (!f) throw std::runtime_error("Lecture records échouée: " + path);

    // Lis (et jette) le bloc de chaînes
    if (hdr.stringBlockSize)
    {
        f.seekg(hdr.stringBlockSize, std::ios::cur);
        if (!f) throw std::runtime_error("Lecture string block échouée: " + path);
    }

    // Parse : on lit au moins les 14 premières colonnes; si le DBC a plus de colonnes,
    // on ignore les supplémentaires (recordSize/fieldCount > 14).
    const uint32_t FIELDS_NEEDED = 14;

    byId.clear();
    byId.reserve(hdr.recordCount);

    for (uint32_t i = 0; i < hdr.recordCount; ++i)
    {
        const char* rec = records.data() + static_cast<size_t>(i) * hdr.recordSize;

        FactionTemplateEntry e{};
        e.ID = readU32(rec + 0 * 4);
        e.Faction = readU32(rec + 1 * 4);
        e.Flags = readU32(rec + 2 * 4);
        e.FactionGroup = readU32(rec + 3 * 4);
        e.FriendGroup = readU32(rec + 4 * 4);
        e.EnemyGroup = readU32(rec + 5 * 4);
        for (int k = 0; k < 4; ++k) e.EnemyFaction[k] = readU32(rec + (6 + k) * 4);
        for (int k = 0; k < 4; ++k) e.FriendFaction[k] = readU32(rec + (10 + k) * 4);

        // Si le fichier a moins de 14 colonnes, on aurait jeté plus haut.
        // Si le fichier a plus de colonnes, on les ignore proprement.

        if (e.ID != 0) // optionnel : ignorer les lignes vides
            byId.emplace(e.ID, e);
    }
}

FactionTemplateDBC FactionTemplate("dbc/FactionTemplate.dbc");

//FactionTemplateDBC FactionTemplate("dbc/FactionTemplate.dbc");

/*int main()
{
    try {
        FactionTemplateDBC ft("dbc/FactionTemplate.dbc");
        ft.showData(25);
        ft.showData(350);
    }
    catch (const std::exception& ex) {
        std::cerr << "Erreur: " << ex.what() << "\n";
        return 2;
    }
}*/