#ifndef FRAME_INTERPOLATION_H
#define FRAME_INTERPOLATION_H

#include "colorimetry.h"

// Constants for frame interpolation
const float MOTION_THRESHOLD = 0.05;      // Threshold for considering pixels "moving" - adjustable
const float BLEND_FACTOR = 0.5;           // How much to blend between frames (0.5 = halfway)

// Structure to hold previous frame data
struct FrameHistory {
    sampler2D previousFrame;       // Previous frame texture
    vec2 previousFrameSize;        // Size of the previous frame
};

// Simple motion estimation between two frames
// Returns the estimated motion vector for the given pixel
vec2 estimateMotion(sampler2D currentFrame, sampler2D previousFrame, vec2 uv, float searchRadius) {
    vec2 bestOffset = vec2(0.0);
    float bestDiff = 1e10;
    vec3 centerColor = texture(currentFrame, uv).rgb;
    
    // Simple block matching algorithm to find motion
    // This is a basic implementation - production would use hierarchical/more sophisticated approach
    const int steps = 8;
    for (int y = -steps; y <= steps; y++) {
        for (int x = -steps; x <= steps; x++) {
            vec2 offset = vec2(x, y) * (searchRadius / float(steps));
            vec2 testUv = uv + offset;
            
            // Skip if we're out of bounds
            if (testUv.x < 0.0 || testUv.x > 1.0 || testUv.y < 0.0 || testUv.y > 1.0)
                continue;
                
            vec3 testColor = texture(previousFrame, testUv).rgb;
            float diff = length(centerColor - testColor);
            
            if (diff < bestDiff) {
                bestDiff = diff;
                bestOffset = offset;
            }
        }
    }
    
    // Return the motion vector (negative because we're looking backwards)
    return -bestOffset;
}

// Performs frame interpolation using motion vectors
vec4 interpolateFrames(sampler2D currentFrame, sampler2D previousFrame, vec2 uv, float mixFactor) {
    // Calculate search radius based on potential movement speed (can be tuned)
    float searchRadius = 0.03;
    
    // Estimate motion vector from current to previous frame
    vec2 motionVector = estimateMotion(currentFrame, previousFrame, uv, searchRadius);
    
    // Skip interpolation for small movements (reduces artifacts in static regions)
    float motionMagnitude = length(motionVector);
    if (motionMagnitude < MOTION_THRESHOLD) {
        return texture(currentFrame, uv);
    }
    
    // Sample from both frames with motion offset
    vec4 current = texture(currentFrame, uv);
    vec4 previous = texture(previousFrame, uv + motionVector * mixFactor);
    
    // Detect occlusions (where motion estimation likely failed)
    // Simple implementation - check color difference as occlusion indicator
    vec4 directPrevious = texture(previousFrame, uv);
    float colorDiff = length(current.rgb - directPrevious.rgb);
    float occlusionFactor = smoothstep(0.1, 0.3, colorDiff);
    
    // Reduce interpolation weight in occlusion areas
    float finalMixFactor = mix(mixFactor, 0.0, occlusionFactor);
    
    // Blend between frames
    return mix(current, previous, finalMixFactor);
}

#endif // FRAME_INTERPOLATION_H