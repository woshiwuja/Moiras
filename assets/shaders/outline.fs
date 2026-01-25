#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec2 resolution;

// Output fragment color
out vec4 finalColor;

void main()
{
    vec2 texelSize = 1.0 / resolution;
    
    // Sample the center pixel
    vec4 center = texture(texture0, fragTexCoord);
    
    // Sobel operator kernels
    float sobelX[9];
    sobelX[0] = -1.0; sobelX[1] = 0.0; sobelX[2] = 1.0;
    sobelX[3] = -2.0; sobelX[4] = 0.0; sobelX[5] = 2.0;
    sobelX[6] = -1.0; sobelX[7] = 0.0; sobelX[8] = 1.0;
    
    float sobelY[9];
    sobelY[0] = -1.0; sobelY[1] = -2.0; sobelY[2] = -1.0;
    sobelY[3] =  0.0; sobelY[4] =  0.0; sobelY[5] =  0.0;
    sobelY[6] =  1.0; sobelY[7] =  2.0; sobelY[8] =  1.0;
    
    // Sample neighboring pixels
    vec3 samples[9];
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            vec2 offset = vec2(float(j - 1), float(i - 1)) * texelSize;
            samples[i * 3 + j] = texture(texture0, fragTexCoord + offset).rgb;
        }
    }
    
    // Calculate gradients
    vec3 gradX = vec3(0.0);
    vec3 gradY = vec3(0.0);
    
    for (int i = 0; i < 9; i++) {
        gradX += samples[i] * sobelX[i];
        gradY += samples[i] * sobelY[i];
    }
    
    // Calculate edge magnitude
    float edgeMagnitude = length(vec2(length(gradX), length(gradY)));
    
    // Threshold for edge detection
    float threshold = 0.5;
    float edge = step(threshold, edgeMagnitude);
    
    // Outline color (black)
    vec3 outlineColor = vec3(0.0);
    
    // Mix original color with outline
    vec3 finalRGB = mix(center.rgb, outlineColor, edge);
    
    finalColor = vec4(finalRGB, center.a);
}
