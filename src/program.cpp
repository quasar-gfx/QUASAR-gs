/*
    Copyright (c) 2024 Anthony J. Thibault
    This software is licensed under the MIT License. See LICENSE for more details.
*/

#include <program.h>

#include <cassert>
#include <iostream>
#include <memory>
#include <sstream>

#include <Utils/Platform.h>

#include <util.h>

#ifndef NDEBUG
#define WARNINGS_AS_ERRORS
#endif

static std::string ExpandMacros(std::vector<std::pair<std::string, std::string>> macros, const std::string& source)
{
    std::string result = source;

    for (const auto& macro : macros)
    {
        std::string::size_type pos = 0;
        while ((pos = result.find(macro.first, pos)) != std::string::npos)
        {
            result.replace(pos, macro.first.length(), macro.second);
            // Move past the last replaced position
            pos += macro.second.length();
        }
    }

    return result;
}

static void DumpShaderSource(const std::string& source)
{
    std::stringstream ss(source);
    std::string line;
    int i = 1;
    while (std::getline(ss, line))
    {
        spdlog::debug("%04d: {}\n", i, line);
        i++;
    }
    spdlog::debug("");
}

static bool CompileShader(GLenum type, const std::string& source, GLint* shaderOut, const std::string& debugName)
{
    GLint shader = glCreateShader(type);
    int size = static_cast<int>(source.size());
    const GLchar* sourcePtr = source.c_str();
    glShaderSource(shader, 1, (const GLchar**)&sourcePtr, &size);
    glCompileShader(shader);

    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled)
    {
        spdlog::error("shader compilation error for \"{}\"!\n", debugName);
    }

    GLint bufferLen = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &bufferLen);
    if (bufferLen > 1)
    {
        if (compiled)
        {
            spdlog::error("shader compilation warning for \"{}\"!\n", debugName);
        }

        GLsizei len = 0;
        std::unique_ptr<char> buffer(new char[bufferLen]);
        glGetShaderInfoLog(shader, bufferLen, &len, buffer.get());
        spdlog::error("{}\n", buffer.get());
        DumpShaderSource(source);
    }

#ifdef WARNINGS_AS_ERRORS
    if (!compiled || bufferLen > 1)
#else
    if (!compiled)
#endif
    {
        return false;
    }

    *shaderOut = shader;
    return true;
}

Program::Program() : vertShader(0), geomShader(0), fragShader(0), computeShader(0), quasar::ShaderBase()
{
#ifdef __ANDROID__
    AddMacro("HEADER", "#version 320 es\nprecision highp float;");
#else
    AddMacro("HEADER", "#version 460");
#endif
}

Program::~Program()
{
    Delete();
}

void Program::AddMacro(const std::string& key, const std::string& value)
{
    // In order to keep the glsl code compiling if the macro is not applied.
    // the key is enclosed in a c-style comment and double %.
    std::string token = "/*%%" + key + "%%*/";
    macros.push_back(std::pair(token, value));
}

bool Program::LoadVertFrag(const std::string& vertFilename, const std::string& fragFilename)
{
    return LoadVertGeomFrag(vertFilename, std::string(), fragFilename);
}

bool Program::LoadVertGeomFrag(const std::string& vertFilename, const std::string& geomFilename, const std::string& fragFilename)
{
    // Delete old shader/ID
    Delete();

    const bool useGeomShader = !geomFilename.empty();

    if (useGeomShader)
    {
        debugName = vertFilename + " + " + geomFilename + " + " + fragFilename;
    }
    else
    {
        debugName = vertFilename + " + " + fragFilename;
    }

    std::string vertSource, geomSource, fragSource;
    if (!LoadFile(vertFilename, vertSource))
    {
        spdlog::error("Failed to load vertex shader {}\n", vertFilename);
        return false;
    }
    vertSource = ExpandMacros(macros, vertSource);

    if (useGeomShader)
    {
        if (!LoadFile(geomFilename, geomSource))
        {
            spdlog::error("Failed to load geometry shader {}\n", geomFilename);
            return false;
        }
        geomSource = ExpandMacros(macros, geomSource);
    }

    if (!LoadFile(fragFilename, fragSource))
    {
        spdlog::error("Failed to load fragment shader \"{}\"\n", fragFilename);
        return false;
    }
    fragSource = ExpandMacros(macros, fragSource);

    if (!CompileShader(GL_VERTEX_SHADER, vertSource, &vertShader, vertFilename))
    {
        spdlog::error("Failed to compile vertex shader \"{}\"\n", vertFilename);
        return false;
    }

    if (useGeomShader)
    {
        geomSource = ExpandMacros(macros, geomSource);
        if (!CompileShader(GL_GEOMETRY_SHADER, geomSource, &geomShader, geomFilename))
        {
            spdlog::error("Failed to compile geometry shader \"{}\"\n", geomFilename);
            return false;
        }
    }

    if (!CompileShader(GL_FRAGMENT_SHADER, fragSource, &fragShader, fragFilename))
    {
        spdlog::error("Failed to compile fragment shader \"{}\"\n", fragFilename);
        return false;
    }

    ID = glCreateProgram();
    glAttachShader(ID, vertShader);
    glAttachShader(ID, fragShader);
    if (useGeomShader)
    {
        glAttachShader(ID, geomShader);
    }
    glLinkProgram(ID);

    if (!CheckLinkStatus())
    {
        spdlog::error("Failed to link ID \"{}\"\n", debugName);

        // dump shader source for reference
        spdlog::debug("");
        spdlog::debug("{} =\n", vertFilename);
        DumpShaderSource(vertSource);
        if (useGeomShader)
        {
            spdlog::debug("{} =\n", geomFilename);
            DumpShaderSource(geomSource);
        }
        spdlog::debug("{} =\n", fragFilename);
        DumpShaderSource(fragSource);

        return false;
    }

    const int MAX_NAME_SIZE = 1028;
    static char name[MAX_NAME_SIZE];

    GLint numAttribs;
    glGetProgramiv(ID, GL_ACTIVE_ATTRIBUTES, &numAttribs);
    for (int i = 0; i < numAttribs; ++i)
    {
        Variable v;
        GLsizei strLen;
        glGetActiveAttrib(ID, i, MAX_NAME_SIZE, &strLen, &v.size, &v.type, name);
        v.loc = glGetAttribLocation(ID, name);
        attribs[name] = v;
    }

    GLint numUniforms;
    glGetProgramiv(ID, GL_ACTIVE_UNIFORMS, &numUniforms);
    for (int i = 0; i < numUniforms; ++i)
    {
        Variable v;
        GLsizei strLen;
        glGetActiveUniform(ID, i, MAX_NAME_SIZE, &strLen, &v.size, &v.type, name);
        int loc = glGetUniformLocation(ID, name);
        v.loc = loc;
        uniforms[name] = v;
    }

    return true;
}

bool Program::LoadCompute(const std::string& computeFilename)
{
    // Delete old shader/ID
    Delete();

    debugName = computeFilename;

    GL_ERROR_CHECK("Program::LoadCompute begin");

    std::string computeSource;
    if (!LoadFile(computeFilename, computeSource))
    {
        spdlog::error("Failed to load compute shader \"{}\"\n", computeFilename);
        return false;
    }

    GL_ERROR_CHECK("Program::LoadCompute LoadFile");

    computeSource = ExpandMacros(macros, computeSource);
    if (!CompileShader(GL_COMPUTE_SHADER, computeSource, &computeShader, computeFilename))
    {
        spdlog::error("Failed to compile compute shader \"{}\"\n", computeFilename);
        return false;
    }

    GL_ERROR_CHECK("Program::LoadCompute CompileShader");

    ID = glCreateProgram();
    glAttachShader(ID, computeShader);
    glLinkProgram(ID);

    GL_ERROR_CHECK("Program::LoadCompute Attach and Link");

    if (!CheckLinkStatus())
    {
        spdlog::error("Failed to link ID \"{}\"\n", debugName);

        // dump shader source for reference
        spdlog::debug("");
        spdlog::debug("{} =\n", computeFilename);
        DumpShaderSource(computeSource);

        return false;
    }

    const int MAX_NAME_SIZE = 1028;
    static char name[MAX_NAME_SIZE];

    GLint numUniforms;
    glGetProgramiv(ID, GL_ACTIVE_UNIFORMS, &numUniforms);
    for (int i = 0; i < numUniforms; ++i)
    {
        Variable v;
        GLsizei strLen;
        glGetActiveUniform(ID, i, MAX_NAME_SIZE, &strLen, &v.size, &v.type, name);
        int loc = glGetUniformLocation(ID, name);
        v.loc = loc;
        uniforms[name] = v;
    }

    GL_ERROR_CHECK("Program::LoadCompute get uniforms");

    // TODO: build reflection info on shader storage blocks

    return true;
}

void Program::Bind() const
{
    bind();
}

int Program::GetUniformLoc(const std::string& name) const
{
    auto iter = uniforms.find(name);
    if (iter != uniforms.end())
    {
        return iter->second.loc;
    }
    else
    {
        assert(false);
        spdlog::warn("Could not find uniform \"{}\" for ID \"{}\"\n", name, debugName);
        return 0;
    }
}

int Program::GetAttribLoc(const std::string& name) const
{
    auto iter = attribs.find(name);
    if (iter != attribs.end())
    {
        return iter->second.loc;
    }
    else
    {
        spdlog::warn("Could not find attrib \"{}\" for ID \"{}\"\n", name, debugName);
        assert(false);
        return 0;
    }
}

void Program::SetUniformRaw(int loc, uint32_t value) const
{
    glUniform1ui(loc, value);
}

void Program::SetUniformRaw(int loc, int32_t value) const
{
    glUniform1i(loc, value);
}

void Program::SetUniformRaw(int loc, float value) const
{
    glUniform1f(loc, value);
}

void Program::SetUniformRaw(int loc, const glm::vec2& value) const
{
    glUniform2fv(loc, 1, (float*)&value);
}

void Program::SetUniformRaw(int loc, const glm::vec3& value) const
{
    glUniform3fv(loc, 1, (float*)&value);
}

void Program::SetUniformRaw(int loc, const glm::vec4& value) const
{
    glUniform4fv(loc, 1, (float*)&value);
}

void Program::SetUniformRaw(int loc, const glm::mat2& value) const
{
    glUniformMatrix2fv(loc, 1, GL_FALSE, (float*)&value);
}

void Program::SetUniformRaw(int loc, const glm::mat3& value) const
{
    glUniformMatrix3fv(loc, 1, GL_FALSE, (float*)&value);
}

void Program::SetUniformRaw(int loc, const glm::mat4& value) const
{
    glUniformMatrix4fv(loc, 1, GL_FALSE, (float*)&value);
}

void Program::SetAttribRaw(int loc, float* values, size_t stride) const
{
    glVertexAttribPointer(loc, 1, GL_FLOAT, GL_FALSE, (GLsizei)stride, values);
    glEnableVertexAttribArray(loc);
}

void Program::SetAttribRaw(int loc, glm::vec2* values, size_t stride) const
{
    glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, (GLsizei)stride, values);
    glEnableVertexAttribArray(loc);
}

void Program::SetAttribRaw(int loc, glm::vec3* values, size_t stride) const
{
    glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, (GLsizei)stride, values);
    glEnableVertexAttribArray(loc);
}

void Program::SetAttribRaw(int loc, glm::vec4* values, size_t stride) const
{
    glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, (GLsizei)stride, values);
    glEnableVertexAttribArray(loc);
}

void Program::Delete()
{
    debugName = "";

    if (vertShader > 0)
    {
        glDeleteShader(vertShader);
        vertShader = 0;
    }

    if (geomShader > 0)
    {
        glDeleteShader(geomShader);
        geomShader = 0;
    }

    if (fragShader > 0)
    {
        glDeleteShader(fragShader);
        fragShader = 0;
    }

    if (computeShader > 0)
    {
        glDeleteShader(computeShader);
        computeShader = 0;
    }

    if (ID > 0)
    {
        glDeleteProgram(ID);
        ID = 0;
    }

    uniforms.clear();
    attribs.clear();
}

bool Program::CheckLinkStatus()
{
    GLint linked;
    glGetProgramiv(ID, GL_LINK_STATUS, &linked);

    if (!linked)
    {
        spdlog::error("Failed to link shaders \"{}\"\n", debugName);
    }

    const GLint MAX_BUFFER_LEN = 4096;
    GLsizei bufferLen = 0;
    std::unique_ptr<char> buffer(new char[MAX_BUFFER_LEN]);
    glGetProgramInfoLog(ID, MAX_BUFFER_LEN, &bufferLen, buffer.get());
    if (bufferLen > 0)
    {
        if (linked)
        {
            spdlog::warn("Warning during linking shaders \"{}\"\n", debugName);
        }
        spdlog::warn("{}\n", buffer.get());
    }

#ifdef WARNINGS_AS_ERRORS
    if (!linked || bufferLen > 1)
#else
    if (!linked)
#endif
    {
        return false;
    }

    return true;
}
