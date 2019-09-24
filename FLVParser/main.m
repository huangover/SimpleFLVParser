//
//  main.m
//  FLVParser
//
//  Created by SihangHuang on 2019/9/23.
//  Copyright © 2019 SihangHuang. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "FLVParser.h"

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // insert code here...
        const char* fileName = "sample.flv";
        if (initWithFile(fileName)) {
            parse();
        }
    }
    return 0;
}
