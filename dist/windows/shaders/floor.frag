#version 330 core
in vec2 vUV;

out vec4 FragColor;

uniform sampler2D uTexture;
uniform int uCols;
uniform int uRows;
uniform int uFrame;
uniform int uMirror;
uniform int uUseColor; // if 1, output uColor instead of sampling texture
uniform vec4 uColor;
uniform float uShadowInner; // inner radius (0..0.5) where alpha==1
uniform float uShadowOuter; // outer radius (0..0.5) where alpha==0

void main() {
    float cols = float(max(uCols, 1));
    float rows = float(max(uRows, 1));
    int frame = max(uFrame, 0);
    
    float col = float(frame % uCols);
    float row = float(frame / uCols);
    
    vec2 frameSize = vec2(1.0 / cols, 1.0 / rows);
    
    // Apply mirroring to UV X coordinate
    vec2 uv = vUV;
    if (uMirror == -1) {
        uv.x = 1.0 - uv.x;
    }
    
    uv = uv * frameSize + vec2(col * frameSize.x, row * frameSize.y);

    if (uUseColor == 1) {
        FragColor = uColor;
    } else if (uUseColor == 2) {
        // radial soft shadow in UV space (quad UV from 0..1)
        vec2 center = vec2(0.5, 0.5);
        float d = distance(uv, center);
        // clamp radii
        float inner = clamp(uShadowInner, 0.0, 0.5);
        float outer = clamp(uShadowOuter, 0.0, 0.5);
        // smooth fade from inner (alpha 1) to outer (alpha 0)
        float a = 0.0;
        if (outer <= inner) {
            a = step(d, inner);
        } else {
            a = 1.0 - smoothstep(inner, outer, d);
        }
        FragColor = vec4(uColor.rgb, uColor.a * a);
    } else {
        // multiply sampled texture by uColor (allows tint/alpha modulation)
        FragColor = texture(uTexture, uv) * uColor;
    }
}
