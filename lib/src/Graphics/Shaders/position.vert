#version 450 core

layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;
layout(push_constant) uniform uPushConstant { vec2 uScale; vec2 uTranslate; } pc;

out gl_PerVertex { vec4 gl_Position; };
layout(location = 0) out struct { vec4 Color; vec2 TexCoord; } Out;

void main()
{
    gl_Position = vec4(aPosition * pc.uScale + pc.uTranslate, 0, 1);
    Out.Color = aColor;
    Out.TexCoord = aTexCoord;
}