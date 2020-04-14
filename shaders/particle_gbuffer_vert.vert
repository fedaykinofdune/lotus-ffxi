#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 proj;
    mat4 view;
    mat4 proj_inverse;
    mat4 view_inverse;
} ubo;

layout(binding = 2) uniform ModelUBO {
    mat4 model;
    mat3 model_IT;
} model;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragPos;
layout(location = 3) out vec3 normal;

void main() {
    gl_Position = ubo.proj * ubo.view * model.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;

    fragPos = (model.model * vec4(inPosition, 1.0)).xyz;
    normal = normalize(model.model_IT * inNormal);
}
