#version 330

// Input fragment attributes (from fragment shader)
in vec2 fragTexCoord;
in float height;

// Output fragment color
out vec4 finalColor;

void main()
{
    vec4 darkblue = vec4(0.0, 0.13, 0.2, 0.8);
    vec4 lightblue = vec4(1.0, 1.0, 1.0, 0.8);
    // interplate between two colors based on height
    finalColor = mix(darkblue, lightblue, height);
}
