#pragma once
using namespace geode::prelude;

using EncStr = std::array<std::string, 3>;

class GDriveEncypt : public CCObject
{
  public:
    static GDriveEncypt *create();

    EncStr encryptString(const std::string_view data); // returns: {raw, iv, tag}
    std::string decryptString(const EncStr &data);

  private:
    std::string getHardwareID();
};
