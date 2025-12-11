#version 330 core
in vec2 vUV;

out vec4 FragColor;

uniform sampler2D uTexture;
uniform int uCols;
uniform int uRows;
uniform int uFrame;
uniform int uMirror;

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
    
    FragColor = texture(uTexture, uv);
}
