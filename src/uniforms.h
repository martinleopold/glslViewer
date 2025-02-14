#pragma once

#include <map>
#include <queue>
#include <array>
#include <vector>
#include <string>
#include <functional>

#include "tools/files.h"
#include "tools/tracker.h"

#include "vera/types/scene.h"

struct CameraData {
    glm::mat4   projection;
    glm::mat4   transform;
};

typedef std::vector<CameraData> CameraPath;

typedef std::array<float, 4> UniformValue;

struct UniformData {
    std::string getType();

    void    set(const UniformValue &_value, size_t _size, bool _int);
    void    parse(const std::vector<std::string>& _command, size_t _start = 1);
    bool    check();

    std::queue<UniformValue>            queue;
    UniformValue                        value;
    size_t                              size    = 0;
    bool                                bInt    = false;
    bool                                change  = false;
};

struct UniformFunction {
    UniformFunction();
    UniformFunction(const std::string &_type);
    UniformFunction(const std::string &_type, std::function<void(vera::Shader&)> _assign);
    UniformFunction(const std::string &_type, std::function<void(vera::Shader&)> _assign, std::function<std::string()> _print);

    std::function<void(vera::Shader&)>   assign;
    std::function<std::string()>        print;
    std::string                         type;
    bool                                present = false;
};

// Uniforms values types (float, vecs and functions)
typedef std::map<std::string, UniformData>      UniformDataMap;
typedef std::map<std::string, UniformFunction>  UniformFunctionsMap;

// Buffers types
typedef std::vector<vera::Fbo>                  BuffersList;
typedef std::vector<vera::PingPong>             DoubleBuffersList;
typedef std::vector<vera::Pyramid>              PyramidsList;

class Uniforms : public vera::Scene {
public:
    Uniforms();
    virtual ~Uniforms();

    virtual void        clear();
    
    // Change state
    virtual void        flagChange();
    virtual void        unflagChange();
    virtual bool        haveChange();

    // Feed uniforms to a specific shader
    virtual bool        feedTo( vera::Shader *_shader, bool _lights = true, bool _buffers = true);

    // defines
    virtual void        addDefine(const std::string& _define, const std::string& _value);
    virtual void        delDefine(const std::string& _define);
    virtual void        printDefines();

    // Buffers
    BuffersList         buffers;
    DoubleBuffersList   doubleBuffers;
    PyramidsList        pyramids;
    virtual void        printBuffers();
    virtual void        clearBuffers();

    // Ingest new uniforms
    // float, vec2, vec3, vec4 and functions (u_time, u_data, etc.)
    UniformDataMap      data;
    UniformFunctionsMap functions;
    virtual void        set( const std::string& _name, float _value);
    virtual void        set( const std::string& _name, float _x, float _y);
    virtual void        set( const std::string& _name, float _x, float _y, float _z);
    virtual void        set( const std::string& _name, float _x, float _y, float _z, float _w);
    virtual void        checkUniforms( const std::string &_vert_src, const std::string &_frag_src );
    virtual bool        parseLine( const std::string &_line );
    virtual void        clearUniforms();
    virtual void        printAvailableUniforms(bool _non_active);
    virtual void        printDefinedUniforms(bool _csv = false);
    

    CameraPath          cameraPath;
    virtual bool        addCameraPath( const std::string& _name );

    Tracker             tracker;

protected:
    bool                m_change;

};


