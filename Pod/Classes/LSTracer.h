#import <Foundation/Foundation.h>

#import <opentracing/OTTracer.h>
#import "LSSpan.h"

NS_ASSUME_NONNULL_BEGIN

/**
 * The error domain for all OpenTracing-related NSErrors.
 */
FOUNDATION_EXPORT NSString *const LSErrorDomain;
/**
 * OTUnsupportedFormat should be used by `OTTracer#inject:format:carrier:` and
 * `OTTracer#extractWithFormat:carrier:` implementations that don't support the
 * requested carrier format.
 */
FOUNDATION_EXPORT NSInteger LSBackgroundTaskError;

@class LSPBSpan;

/**
 * An implementation of the OTTracer protocol.
 *
 * Either pass the resulting id<OTTracer> around your application explicitly or use the OTGlobal singleton mechanism.
 *
 * LSTracer is thread-safe.
 *
 * @see OTGlobal
 */
@interface LSTracer : NSObject<OTTracer>

#pragma mark - LSTracer initialization

/**
 * @see `-[LSTracer initWithToken:componentName:hostport]` for parameter details.
 *
 * @return An `LSTracer` instance that's ready to create spans and logs.
 */
- (instancetype) initWithToken:(NSString*)accessToken;

/**
 * @see `-[LSTracer initWithToken:componentName:hostport]` for parameter details.
 *
 * @return An `LSTracer` instance that's ready to create spans and logs.
 */
- (instancetype) initWithToken:(NSString*)accessToken
                 componentName:(nullable NSString*)componentName;

/**
 * @see `-[LSTracer initWithToken:componentName:hostport:flushIntervalSeconds:]` for parameter details.
 *
 * @return An `LSTracer` instance that's ready to create spans and logs.
 */
- (instancetype) initWithToken:(NSString*)accessToken
                 componentName:(nullable NSString*)componentName
          flushIntervalSeconds:(NSUInteger)flushIntervalSeconds;

/**
 * Initialize an LSTracer instance. Either pass the resulting LSTracer* around your application explicitly or use the OTGlobal singleton mechanism.
 *
 * Whether calling `-[LSTracer flush]` manually or whether using automatic background flushing, users may wish to register for UIApplicationDidEnterBackgroundNotification notifications and explicitly call flush at that point.
 *
 * @param accessToken the access token.
 * @param componentName the "component name" to associate with spans from this process; e.g., the name of your iOS app or the bundle name.
 * @param hostport the gRPC collector's host and (TLS) port as a single string (e.g.  @"collector-grpc.lightstep.com:443")
 * @param flushIntervalSeconds the flush interval, or 0 for no automatic background flushing
 * @param insecureGRPC if true, disable (TLS) security on the gRPC connection
 *
 * @return An `LSTracer` instance that's ready to create spans and logs.
 *
 * @see OTGlobal
 */
- (instancetype) initWithToken:(NSString*)accessToken
                 componentName:(nullable NSString*)componentName
                      hostport:(nullable NSString*)hostport
          flushIntervalSeconds:(NSUInteger)flushIntervalSeconds
                  insecureGRPC:(BOOL)insecureGRPC;

#pragma mark - OpenTracing API

- (id<OTSpan>) startSpan:(NSString*)operationName;

- (id<OTSpan>) startSpan:(NSString*)operationName
                    tags:(nullable NSDictionary*)tags;

- (id<OTSpan>) startSpan:(NSString*)operationName
                 childOf:(nullable id<OTSpanContext>)parentSpan;

- (id<OTSpan>) startSpan:(NSString*)operationName
                 childOf:(nullable id<OTSpanContext>)parentSpan
                    tags:(nullable NSDictionary*)tags;

- (id<OTSpan>) startSpan:(NSString*)operationName
                 childOf:(nullable id<OTSpanContext>)parentSpan
                    tags:(nullable NSDictionary*)tags
               startTime:(nullable NSDate*)startTime;

- (id<OTSpan>) startSpan:(NSString*)operationName
              references:(nullable NSArray*)references
                    tags:(nullable NSDictionary*)tags
               startTime:(nullable NSDate*)startTime;

- (BOOL) inject:(id<OTSpanContext>)span
         format:(NSString*)format
        carrier:(id)carrier;
- (BOOL) inject:(id<OTSpanContext>)span
         format:(NSString*)format
        carrier:(id)carrier
          error:(NSError* __autoreleasing *)outError;

- (id<OTSpanContext>) extractWithFormat:(NSString*)format
                                carrier:(id)carrier;
- (id<OTSpanContext>) extractWithFormat:(NSString*)format
                                carrier:(id)carrier
                                  error:(NSError* __autoreleasing *)outError;

#pragma mark - LightStep extensions and internal methods

/**
 * The remote service URL string (as derived from `sharedInstancWithServiceHostport:token:`).
 */
@property (nonatomic, readonly) NSString* serviceUrl;

/**
 * The `LSTracer` instance's globally unique id ("guid"), which is both immutable and assigned automatically by LightStep.
 */
@property (nonatomic, readonly) NSString* runtimeGuid;

/**
 * The `LSTracer` instance's maximum number of records to buffer between reports.
 */
@property (atomic) NSUInteger maxSpanRecords;

/**
 * Maximum string length of any single JSON payload.
 */
@property (atomic) NSUInteger maxPayloadJSONLength;


/**
 * Returns true if the library is currently buffering and reporting data.
 */
- (BOOL) enabled;

/**
 * Returns the Tracer's access token.
 */
- (NSString*) accessToken;

/**
 * Record a span.
 */
- (void) _appendSpanRecord:(LSPBSpan*)spanRecord;

/**
 * Flush any buffered data to the collector. Returns without blocking.
 *
 * If non-nil, doneCallback will be invoked once the flush() completes.
 */
- (void) flush:(nullable void (^)(NSError * _Nullable error))doneCallback;

@end

NS_ASSUME_NONNULL_END
