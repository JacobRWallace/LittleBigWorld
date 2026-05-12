#version 330 core
in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

uniform sampler2D textureSampler;

out vec4 FragColor;

void main()
{
    vec3 texColor = texture(textureSampler, TexCoords).rgb;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    float diff = max(dot(norm, lightDir), 0.0);

    vec3 ambient = vec3(0.2, 0.2, 0.2);
    vec3 diffuse = diff * vec3(0.8, 0.8, 0.8);
    vec3 result = (ambient + diffuse) * texColor;

    FragColor = vec4(result, 1.0);
}
