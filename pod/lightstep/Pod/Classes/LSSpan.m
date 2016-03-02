#import <UIKit/UIKit.h>
#import "LSSpan.h"
#import "LSTracer.h"
#import "LSUtil.h"

#pragma mark - LSSpan

@implementation LSSpan {
    LSTracer* m_tracer;
    NSString* m_guid;
    NSString* m_operationName;
    NSDate* m_startTime;
    NSMutableDictionary* m_tags;
    NSMutableDictionary* m_baggage;
    bool m_errorFlag;
}

- (instancetype) initWithTracer:(LSTracer*)client {
    return [self initWithTracer:client
                  operationName:@""
                         parent:nil
                           tags:nil
                      startTime:nil];
}

- (instancetype) initWithTracer:(LSTracer*)client
                  operationName:(NSString*)operationName
                         parent:(LSSpan*)parent
                           tags:(NSDictionary*)tags
                      startTime:(NSDate*)startTime {
    if (self = [super init]) {
        self->m_tracer = client;
        self->m_guid = [LSUtil generateGUID];
        self->m_operationName = operationName;
        self->m_startTime = startTime;
        self->m_tags = [NSMutableDictionary dictionary];
        self->m_baggage = [NSMutableDictionary dictionary];
        self->m_errorFlag = false;

        if (startTime == nil) {
            m_startTime = [NSDate date];
        }
        if (parent != nil) {
            [m_tags setObject:parent->m_guid forKey:@"parent_span_guid"];
        }
        [self _addTags:tags];
    }
    return self;
}

- (void) dealloc {
}

- (LSTracer*) tracer {
    // The m_tracer pointer is immutable after initialization; no locking required.
    return m_tracer;
}

- (void) setOperationName:(NSString *)operationName {
    @synchronized(self) {
        m_operationName = operationName;
    }
}

- (void) setTag:(NSString *)key value:(NSString *)value {
    @synchronized(self) {
        [m_tags setObject:value forKey:key];
    }
}

- (void)logEvent:(NSString*)eventName {
    [self log:eventName timestamp:[NSDate date] payload:nil];
}

- (void)logEvent:(NSString*)eventName payload:(NSObject*)payload {
    [self log:eventName timestamp:[NSDate date] payload:payload];
}

- (void)log:(NSString*)eventName
  timestamp:(NSDate*)timestamp
    payload:(NSObject*)payload {

    // No locking is requied as all the member variables used below are immutable
    // after initialization:
    // - m_tracer
    // - m_guid

    if (![m_tracer enabled]) {
        return;
    }

    NSString* payloadJSON = [LSUtil objectToJSONString:payload
                                             maxLength:[m_tracer maxPayloadJSONLength]];
    RLLogRecord* logRecord = [[RLLogRecord alloc]
                              initWithTimestamp_micros:[timestamp toMicros]
                              runtime_guid:[m_tracer runtimeGuid]
                              span_guid:m_guid
                              stable_name:eventName
                              message:nil
                              level:@"I"
                              thread_id:(int64_t)[NSThread currentThread]
                              filename:nil
                              line_number:0
                              stack_frames:nil
                              payload_json:payloadJSON
                              error_flag:false];

    [m_tracer _appendLogRecord:logRecord];
}

- (void)setBaggageItem:(NSString*)key value:(NSString*)value {
    // TODO: need to check the key/value constraints
    @synchronized(self) {
        [m_baggage setObject:value forKey:key];
    }
}

- (NSString*)getBaggageItem:(NSString*)key {
    @synchronized(self) {
        id obj = [m_baggage objectForKey:key];
        return (NSString*)obj;
    }
}

- (void) finish {
    [self finishWithTime:[NSDate date]];
}

- (void) finishWithTime:(NSDate *)finishTime {
    if (finishTime == nil) {
        finishTime = [NSDate date];
    }

    RLSpanRecord* record;
    @synchronized(self) {
        NSMutableArray* tagArray;
        if ([m_tags count] > 0) {
            tagArray = [[NSMutableArray alloc] initWithCapacity:[m_tags count]];
            for (NSString* key in m_tags ) {
                RLKeyValue* pair = [[RLKeyValue alloc] initWithKey:key Value:m_tags[key]];
                [tagArray addObject:pair];
            }
        }
        record =[[RLSpanRecord alloc] initWithSpan_guid:m_guid
                                           runtime_guid:m_tracer.runtimeGuid
                                              span_name:m_operationName
                                               join_ids:nil
                                          oldest_micros:[m_startTime toMicros]
                                        youngest_micros:[finishTime toMicros]
                                             attributes:tagArray
                                             error_flag:m_errorFlag];
    }
    [m_tracer _appendSpanRecord:record];
}

- (void)_addTags:(NSDictionary*)tags {
    if (tags == nil) {
        return;
    }
    @synchronized(self) {
        [m_tags addEntriesFromDictionary:tags];
    }
}

- (LSSpan*)_startChildSpan:(NSString*)operationName
                      tags:(NSDictionary*)tags
                 startTime:(NSDate*)startTime {

    return [[LSSpan alloc] initWithTracer:m_tracer
                            operationName:operationName
                                   parent:nil
                                     tags:tags
                                startTime:startTime];
}

@end