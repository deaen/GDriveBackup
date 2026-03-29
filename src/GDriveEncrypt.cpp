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

#if defined(GEODE_IS_WINDOWS)
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
#elif defined(GEODE_IS_ANDROID)
#include <Geode/cocos/platform/android/jni/JniHelper.h>

std::string GDriveEncypt::getHardwareID()
{
    std::string hardwareID;
    JniMethodInfo t;

    if (JniHelper::getStaticMethodInfo(t, "com/customRobTop/BaseRobTopActivity", "getUserID", "()Ljava/lang/String;"))
    {
        jstring str = reinterpret_cast<jstring>(t.env->CallStaticObjectMethod(t.classID, t.methodID));
        hardwareID = JniHelper::jstring2string(str);

        t.env->DeleteLocalRef(t.classID);
        t.env->DeleteLocalRef(str);
    }
    else
    {
        auto vm = cocos2d::JniHelper::getJavaVM();

        JNIEnv *env;
        if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) == JNI_OK)
        {
            env->ExceptionClear();
        }
    }

    return hardwareID;
}
#elif defined(GEODE_IS_MACOS)
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

std::string GDriveEncypt::getHardwareID()
{
    std::string hardwareID;

    io_service_t platformExpert =
        IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("IOPlatformExpertDevice"));
    if (!platformExpert)
        return "";

    CFTypeRef uuidCF = IORegistryEntryCreateCFProperty(platformExpert, CFSTR("IOPlatformUUID"), kCFAllocatorDefault, 0);

    IOObjectRelease(platformExpert);
    if (!uuidCF)
        return "";

    char buffer[256];

    if (CFStringGetCString((CFStringRef)uuidCF, buffer, sizeof(buffer), kCFStringEncodingUTF8))
    {
        hardwareID = buffer;
    }

    CFRelease(uuidCF);
    return hardwareID;
}

#elif defined(GEODE_IS_IOS)
std::string GDriveEncypt::getHardwareID()
{
    return iosGetHardwareID();
}
#endif