
#import <Foundation/NSObjCRuntime.h>
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

extern int add(int, int);

@interface FCKViewController : UIViewController

@end

@implementation FCKViewController

- (void)viewDidLoad __attribute__((annotate("0.0.7.1")))
{
    [super viewDidLoad];

    id object = [[NSObject alloc] init];
    if (object) {
        [self add:1 b:1];
    } else {
        NSLog(@"alloc error");
    }
}

- (int)add:(int)a b:(int)b __attribute__((annotate("0.0.7.1")))
{
    int result = add(a, b);

    return result;
}

@end
