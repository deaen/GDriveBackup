#import <UIKit/UIKit.h>

extern "C" std::string iosGetHardwareID()
{
    @autoreleasepool {
        NSString* hardwareID = [[[UIDevice currentDevice] identifierForVendor] UUIDString];
        return std::string([hardwareID UTF8String]);
    }
}