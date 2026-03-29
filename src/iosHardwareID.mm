#import <UIKit/UIKit.h>
#include <string>

// Untested...
std::string iosGetHardwareID()
{
    @autoreleasepool
    {
        NSString *hardwareID = [[[UIDevice currentDevice] identifierForVendor] UUIDString];
        return std::string([hardwareID UTF8String]);
    }
}