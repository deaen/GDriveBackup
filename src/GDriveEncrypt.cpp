#include "GDriveEncrypt.hpp"
#include "picosha2.h"
#include "plusaes.hpp"

GDriveEncypt *GDriveEncypt::create()
{
    auto ret = new GDriveEncypt();
    ret->autorelease();
    return ret;
}

EncStr GDriveEncypt::encryptString(const std::string &data)
{
    const std::string raw = data;

    std::vector<unsigned char> key(picosha2::k_digest_size);
    picosha2::hash256(getHardwareID(), key);

    const std::string iv = utils::random::generateHexString(12);
    unsigned char tag[16];

    // encrypt
    bool error = plusaes::encrypt_gcm((unsigned char *)raw.data(), raw.size(), nullptr, 0, &key[0], key.size(),
                                      (const unsigned char (*)[12])iv.data(), &tag);

    if (error)
    {
        return EncStr{};
    }

    return {raw, iv, std::string(reinterpret_cast<const char *>(tag))};
}

std::string GDriveEncypt::decryptString(const EncStr &data)
{
    const std::string raw = data[0];
    const std::string iv = data[1];
    const std::string tag = data[2];

    std::vector<unsigned char> key(picosha2::k_digest_size);
    picosha2::hash256(getHardwareID(), key);
    
    // decrypt
    bool error = plusaes::decrypt_gcm((unsigned char *)raw.data(), raw.size(), nullptr, 0, &key[0], key.size(),
                                      (const unsigned char (*)[12])iv.data(), (const unsigned char (*)[16])tag.data());

    if (error)
    {
        return "";
    }

    return raw;
}

#ifdef GEODE_IS_WINDOWS
#include <windows.h>
std::string GDriveEncypt::getHardwareID()
{
    std::string hardwareID;
    /* Get GUID */
    std::array<char, 64> buffer{};
    DWORD size = buffer.size();
    HKEY hKey;

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY, &hKey) ==
        ERROR_SUCCESS)
    {
        if (RegQueryValueExA(hKey, "MachineGuid", nullptr, nullptr, (LPBYTE)buffer.data(), &size) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            hardwareID += std::string(buffer.data());
        }
        RegCloseKey(hKey);
    }
    /* Get C Drive serial */
    DWORD serial = 0;
    GetVolumeInformationA("C:\\", nullptr, 0, &serial, nullptr, nullptr, nullptr, 0);
    hardwareID += std::to_string(serial);

    return hardwareID;
}
#endif
