#import <Foundation/NSObjCRuntime.h>
#import <Foundation/Foundation.h>

// extern void cfun(int, double);

// typedef struct AAA {
// 	int a;
// } AAA;

// typedef struct BBB {
// 	AAA aaa;
// 	int *btn;
// } BBB;

@implementation FCKViewController

- (void)a:(int)ccccc b:(int)dddd
{}

- (void)b __attribute__((annotate("reflect-class")))
{
    id view = [[NSObject alloc] init];

    // [[NSObject alloc] init];

    // int i = 1;
    // if (i) {
    // 	[[NSObject alloc] init];
    // } else if (i) {
    // 	[[NSObject alloc] init];
    // }

    if (view) {
    	[[NSObject alloc] init];
    } 
  //   else if (view) {
		// [[NSObject alloc] init];
  //   } else if (view) {
  //   	[[NSObject alloc] init];
  //   } else {
  //   	[[NSObject alloc] init];
  //   }

    // if ([[NSObject alloc] init]) {

    // }

	// CGRect frame = CGRectMake(1,1,1,1);

	// id inst = self;
	// [inst a:34534546758678678];
	// NSString str = @"3456";
	// NSLog(@"%@ %f", str, 456.8);
	// cfun(987, 89.6);
}

@end
