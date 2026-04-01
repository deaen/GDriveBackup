#pragma once
using namespace geode::prelude;

using EncStr = std::array<std::string, 3>;

// made this to avoid people stealing/accidently sharing their google tokens in the saved.json that has access to like all their online backups by encrypting it with a hardware ID
class GDriveEncypt : public CCObject
{
  public:
    static GDriveEncypt *create();

    EncStr encryptString(const std::string_view data); // returns: {raw, iv, tag}
    std::string decryptString(const EncStr &data);

  private:
    std::string getHardwareID();
};
