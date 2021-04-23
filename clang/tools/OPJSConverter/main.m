#import <Foundation/NSObjCRuntime.h>

extern void cfun(int, double);

struct AAA {
	int a;
};

struct BBB {
	AAA aaa;
	int *btn;
};

@implementation FCKViewController

- (void)a:(CGRect)c
{}

- (void)b __attribute__((annotate("reflect-class")))
{
	CGRect frame = CGRectMake(1,1,1,1);

	// id inst = self;
	// [inst a:34534546758678678];
	// NSString str = @"3456";
	// NSLog(@"%@ %f", str, 456.8);
	// cfun(987, 89.6);
}

@end
