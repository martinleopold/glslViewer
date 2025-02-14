#include "uniforms.h"

#include <regex>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "tools/text.h"
#include "vera/ops/string.h"
#include "vera/xr/xr.h"

std::string UniformData::getType() {
    if (size == 1) return (bInt ? "int" : "float");
    else return (bInt ? "ivec" : "vec") + vera::toString(size); 
}

void UniformData::set(const UniformValue &_value, size_t _size, bool _int ) {
    bInt = _int;
    size = _size;

    if (change)
        queue.push( _value );
    else
        value = _value;
    
    change = true;
}

void UniformData::parse(const std::vector<std::string>& _command, size_t _start) {;
    UniformValue candidate;
    for (size_t i = _start; i < _command.size() && i < 5; i++)
        candidate[i-_start] = vera::toFloat(_command[i]);

    set(candidate, _command.size() - _start, true);
}

bool UniformData::check() {
    if (queue.empty())
        change = false;
    else {
        value = queue.front();
        queue.pop();
        change = true;
    }
    return change;
}

UniformFunction::UniformFunction() {
    type = "-undefined-";
}

UniformFunction::UniformFunction(const std::string &_type) {
    type = _type;
}

UniformFunction::UniformFunction(const std::string &_type, std::function<void(vera::Shader&)> _assign) {
    type = _type;
    assign = _assign;
}

UniformFunction::UniformFunction(const std::string &_type, std::function<void(vera::Shader&)> _assign, std::function<std::string()> _print) {
    type = _type;
    assign = _assign;
    print = _print;
}


Uniforms::Uniforms() : m_change(false) {

    // IBL
    //
    functions["u_iblLuminance"] = UniformFunction("float", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_iblLuminance", 30000.0f * activeCamera->getExposure());
    },
    [this]() { 
        if (activeCamera)
            return vera::toString(30000.0f * activeCamera->getExposure());
        return std::string("");
    });
    
    // CAMERA UNIFORMS
    //
    functions["u_camera"] = UniformFunction("vec3", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_camera", - (activeCamera->getPosition()) );
    },
    [this]() {
        if (activeCamera)
            return vera::toString(-activeCamera->getPosition(), ',');
        return std::string("");
    });

    functions["u_cameraDistance"] = UniformFunction("float", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_cameraDistance", activeCamera->getDistance());
    },
    [this]() { 
        if (activeCamera)
            return vera::toString(activeCamera->getDistance());
        return std::string("");
    });

    functions["u_cameraNearClip"] = UniformFunction("float", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_cameraNearClip", activeCamera->getNearClip());
    },
    [this]() { 
        if (activeCamera)
            return vera::toString(activeCamera->getNearClip());
        return std::string("");
    });

    functions["u_cameraFarClip"] = UniformFunction("float", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_cameraFarClip", activeCamera->getFarClip());
    },
    [this]() { 
        if (activeCamera)
            return vera::toString(activeCamera->getFarClip()); 
        return std::string("");
    });

    functions["u_cameraEv100"] = UniformFunction("float", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_cameraEv100", activeCamera->getEv100());
    },
    [this]() {
        if (activeCamera)
            return vera::toString(activeCamera->getEv100());
        return std::string("");
    });

    functions["u_cameraExposure"] = UniformFunction("float", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_cameraExposure", activeCamera->getExposure());
    },
    [this]() { 
        if (activeCamera)
            return vera::toString(activeCamera->getExposure());
        return std::string("");
    });

    functions["u_cameraAperture"] = UniformFunction("float", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_cameraAperture", activeCamera->getAperture());
    },
    [this]() { 
        if (activeCamera)
            return vera::toString(activeCamera->getAperture());
        return std::string("");
    });

    functions["u_cameraShutterSpeed"] = UniformFunction("float", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_cameraShutterSpeed", activeCamera->getShutterSpeed());
    },
    [this]() { 
        if (activeCamera)
            return vera::toString(activeCamera->getShutterSpeed());
        return std::string("");
    });

    functions["u_cameraSensitivity"] = UniformFunction("float", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_cameraSensitivity", activeCamera->getSensitivity());
    },
    [this]() { 
        if (activeCamera)
            return vera::toString(activeCamera->getSensitivity());
        return std::string("");
    });

    functions["u_cameraChange"] = UniformFunction("bool", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_cameraChange", activeCamera->bChange);
    },
    [this]() { 
        if (activeCamera)
            return vera::toString(activeCamera->getSensitivity());
        return std::string("");
    });
    
    functions["u_normalMatrix"] = UniformFunction("mat3", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_normalMatrix", activeCamera->getNormalMatrix());
    });

    // CAMERA MATRIX UNIFORMS
    functions["u_viewMatrix"] = UniformFunction("mat4", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_viewMatrix", activeCamera->getViewMatrix());
    });

    functions["u_inverseViewMatrix"] = UniformFunction("mat4", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_inverseViewMatrix", activeCamera->getInverseViewMatrix());
    });

    functions["u_projectionMatrix"] = UniformFunction("mat4", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_projectionMatrix", activeCamera->getProjectionMatrix());
    });

    functions["u_inverseProjectionMatrix"] = UniformFunction("mat4", [this](vera::Shader& _shader) {
        if (activeCamera)
            _shader.setUniform("u_inverseProjectionMatrix", activeCamera->getInverseProjectionMatrix());
    });
}

Uniforms::~Uniforms(){
    clearUniforms();
}

void Uniforms::clear() {
    clearUniforms();
    vera::Scene::clear();
}

bool Uniforms::feedTo(vera::Shader *_shader, bool _lights, bool _buffers ) {
    bool update = false;

    // Pass Native uniforms 
    for (UniformFunctionsMap::iterator it = functions.begin(); it != functions.end(); ++it) {
        if (!_lights && ( it->first == "u_scene" || it->first == "u_sceneDepth" || it->first == "u_sceneNormal" || it->first == "u_scenePosition") )
            continue;

        if (it->second.present)
            if (it->second.assign)
                it->second.assign( *_shader );
    }

    // Pass User defined uniforms
    if (m_change) {
        for (UniformDataMap::iterator it = data.begin(); it != data.end(); ++it) {
            if (it->second.change) {
                _shader->setUniform(it->first, it->second.value.data(), it->second.size);
                update += true;
            }
        }
    }

    // Pass Textures Uniforms
    for (vera::TexturesMap::iterator it = textures.begin(); it != textures.end(); ++it) {
        _shader->setUniformTexture(it->first, it->second, _shader->textureIndex++ );
        _shader->setUniform(it->first+"Resolution", float(it->second->getWidth()), float(it->second->getHeight()));
    }

    for (vera::TextureStreamsMap::iterator it = streams.begin(); it != streams.end(); ++it) {
        for (size_t i = 0; i < it->second->getPrevTexturesTotal(); i++)
            _shader->setUniformTexture(it->first+"Prev["+vera::toString(i)+"]", it->second->getPrevTextureId(i), _shader->textureIndex++);

        _shader->setUniform(it->first+"Time", float(it->second->getTime()));
        _shader->setUniform(it->first+"Fps", float(it->second->getFps()));
        _shader->setUniform(it->first+"Duration", float(it->second->getDuration()));
        _shader->setUniform(it->first+"CurrentFrame", float(it->second->getCurrentFrame()));
        _shader->setUniform(it->first+"TotalFrames", float(it->second->getTotalFrames()));
    }

    // Pass Buffers Texture
    if (_buffers) {
        for (size_t i = 0; i < buffers.size(); i++)
            _shader->setUniformTexture("u_buffer" + vera::toString(i), &buffers[i], _shader->textureIndex++ );

        for (size_t i = 0; i < doubleBuffers.size(); i++)
            _shader->setUniformTexture("u_doubleBuffer" + vera::toString(i), doubleBuffers[i].src, _shader->textureIndex++ );
    }

    // Pass Convolution Piramids resultant Texture
    for (size_t i = 0; i < pyramids.size(); i++)
        _shader->setUniformTexture("u_pyramid" + vera::toString(i), pyramids[i].getResult(), _shader->textureIndex++ );
    
    if (_lights) {
        // Pass Light Uniforms
        if (lights.size() == 1) {
            vera::LightsMap::iterator it = lights.begin();
            _shader->setUniform("u_lightColor", it->second->color);
            _shader->setUniform("u_lightIntensity", it->second->intensity);
            if (it->second->getLightType() != vera::LIGHT_DIRECTIONAL)
                _shader->setUniform("u_light", it->second->getPosition());
            if (it->second->getLightType() == vera::LIGHT_DIRECTIONAL || it->second->getLightType() == vera::LIGHT_SPOT)
                _shader->setUniform("u_lightDirection", it->second->direction);
            if (it->second->falloff > 0)
                _shader->setUniform("u_lightFalloff", it->second->falloff);
            _shader->setUniform("u_lightMatrix", it->second->getBiasMVPMatrix() );
            _shader->setUniformDepthTexture("u_lightShadowMap", it->second->getShadowMap(), _shader->textureIndex++ );
        }
        else {
            // TODO:
            //      - Lights should be pass as structs?? 

            for (vera::LightsMap::iterator it = lights.begin(); it != lights.end(); ++it) {
                std::string name = "u_" + it->first;

                _shader->setUniform(name + "Color", it->second->color);
                _shader->setUniform(name + "Intensity", it->second->intensity);
                if (it->second->getLightType() != vera::LIGHT_DIRECTIONAL)
                    _shader->setUniform(name, it->second->getPosition());
                if (it->second->getLightType() == vera::LIGHT_DIRECTIONAL || it->second->getLightType() == vera::LIGHT_SPOT)
                    _shader->setUniform(name + "Direction", it->second->direction);
                if (it->second->falloff > 0)
                    _shader->setUniform(name +"Falloff", it->second->falloff);

                _shader->setUniform(name + "Matrix", it->second->getBiasMVPMatrix() );
                _shader->setUniformDepthTexture(name + "ShadowMap", it->second->getShadowMap(), _shader->textureIndex++ );
            }
        }
        
        if (activeCubemap) {
            _shader->setUniformTextureCube("u_cubeMap", (vera::TextureCube*)activeCubemap);
            _shader->setUniform("u_SH", activeCubemap->SH, 9);
        }
    }
    return update;
}

void Uniforms::flagChange() {
    m_change = true;

    if (activeCamera)
        activeCamera->bChange = true;

    // Flag all user uniforms as changed
    for (UniformDataMap::iterator it = data.begin(); it != data.end(); ++it)
        it->second.change = true;
}

void Uniforms::unflagChange() {
    if (m_change) {
        m_change = false;

        // Flag all user uniforms as NOT changed
        for (UniformDataMap::iterator it = data.begin(); it != data.end(); ++it)
            m_change += it->second.check();
    }

    for (vera::LightsMap::iterator it = lights.begin(); it != lights.end(); ++it)
        it->second->bChange = false;

    if (activeCamera)
        activeCamera->bChange = false;
}

bool Uniforms::haveChange() { 
    if (activeCamera)
        if (activeCamera->bChange)
            return true;
            
    if (functions["u_time"].present || 
        functions["u_date"].present ||
        functions["u_delta"].present ||
        functions["u_mouse"].present)
        return true;

    for (vera::LightsMap::const_iterator it = lights.begin(); it != lights.end(); ++it)
        if (it->second->bChange)
            return true;

    if (m_change || streams.size() > 0)
        return true;

    return false;
}


void Uniforms::set(const std::string& _name, float _value) {
    UniformValue value;
    value[0] = _value;
    data[_name].set(value, 1, false);
    m_change = true;
}

void Uniforms::set(const std::string& _name, float _x, float _y) {
    UniformValue value;
    value[0] = _x;
    value[1] = _y;
    data[_name].set(value, 2, false);
    m_change = true;
}

void Uniforms::set(const std::string& _name, float _x, float _y, float _z) {
    UniformValue value;
    value[0] = _x;
    value[1] = _y;
    value[2] = _z;
    data[_name].set(value, 3, false);
    m_change = true;
}

void Uniforms::set(const std::string& _name, float _x, float _y, float _z, float _w) {
    UniformValue value;
    value[0] = _x;
    value[1] = _y;
    value[2] = _z;
    value[3] = _w;
    data[_name].set(value, 4, false);
    m_change = true;
}

bool Uniforms::parseLine( const std::string &_line ) {
    std::vector<std::string> values = vera::split(_line,',');
    if (values.size() > 1) {
        data[ values[0] ].parse(values, 1);
        m_change = true;
        return true;
    }
    return false;
}

void Uniforms::checkUniforms( const std::string &_vert_src, const std::string &_frag_src ) {
    // Check active native uniforms
    for (UniformFunctionsMap::iterator it = functions.begin(); it != functions.end(); ++it) {
        std::string name = it->first + ";";
        std::string arrayName = it->first + "[";
        bool present = ( findId(_vert_src, name.c_str()) || findId(_frag_src, name.c_str()) || findId(_frag_src, arrayName.c_str()) );
        if ( it->second.present != present ) {
            it->second.present = present;
            m_change = true;
        } 
    }
}


void Uniforms::printAvailableUniforms(bool _non_active) {
    if (_non_active) {
        // Print all Native Uniforms (they carry functions)
        for (UniformFunctionsMap::iterator it= functions.begin(); it != functions.end(); ++it) {                
            std::cout << "uniform " << it->second.type << ' ' << it->first << ";";
            if (it->second.print) 
                std::cout << " // " << it->second.print();
            std::cout << std::endl;
        }
    }
    else {
        // Print Native Uniforms (they carry functions) that are present on the shader
        for (UniformFunctionsMap::iterator it= functions.begin(); it != functions.end(); ++it) {                
            if (it->second.present) {
                std::cout<< "uniform " << it->second.type << ' ' << it->first << ";";
                if (it->second.print)
                    std::cout << " // " << it->second.print();
                std::cout << std::endl;
            }
        }
    }
}

void Uniforms::printDefinedUniforms(bool _csv){
    // Print user defined uniform data
    if (_csv) {
        for (UniformDataMap::iterator it= data.begin(); it != data.end(); ++it) {
            std::cout << it->first;
            for (int i = 0; i < it->second.size; i++) {
                std::cout << ',' << it->second.value[i];
            }
            std::cout << std::endl;
        }
    }
    else {
        for (UniformDataMap::iterator it= data.begin(); it != data.end(); ++it) {
            std::cout << "uniform " << it->second.getType() << "  " << it->first << ";";
            for (int i = 0; i < it->second.size; i++)
                std::cout << ((i == 0)? " // " : "," ) << it->second.value[i];
            
            std::cout << std::endl;
        }
    }    
}

void Uniforms::addDefine(const std::string& _define, const std::string& _value) {
    for (vera::ModelsMap::iterator it = models.begin(); it != models.end(); ++it)
        it->second->addDefine(_define, _value);
}

void Uniforms::delDefine(const std::string& _define) {
    for (vera::ModelsMap::iterator it = models.begin(); it != models.end(); ++it)
        it->second->delDefine(_define);
}

void Uniforms::printDefines() {
    for (vera::ModelsMap::iterator it = models.begin(); it != models.end(); ++it) {
        std::cout << std::endl;
        std::cout << it->second->getName() << std::endl;
        std::cout << "-------------- " << std::endl;
        it->second->printDefines();
    }
}

void Uniforms::printBuffers() {
    for (size_t i = 0; i < buffers.size(); i++)
        std::cout << "uniform sampler2D u_buffer" << i << ";" << std::endl;

    for (size_t i = 0; i < doubleBuffers.size(); i++)
        std::cout << "uniform sampler2D u_doubleBuffer" << i << ";" << std::endl;

    for (size_t i = 0; i < pyramids.size(); i++)
        std::cout << "uniform sampler2D u_pyramid" << i << ";" << std::endl;  

    if (functions["u_scene"].present)
        std::cout << "uniform sampler2D u_scene;" << std::endl;

    if (functions["u_sceneDepth"].present)
        std::cout << "uniform sampler2D u_sceneDepth;" << std::endl;

    if (functions["u_scenePosition"].present)
        std::cout << "uniform sampler2D u_scenePosition;" << std::endl;

    if (functions["u_sceneNormal"].present)
        std::cout << "uniform sampler2D u_sceneNormal;" << std::endl;
}

void Uniforms::clearBuffers() {
    buffers.clear();
    doubleBuffers.clear();
    pyramids.clear();
}

void Uniforms::clearUniforms() {
    data.clear();

    for (UniformFunctionsMap::iterator it = functions.begin(); it != functions.end(); ++it)
        it->second.present = false;
}

bool Uniforms::addCameraPath( const std::string& _name ) {
    std::fstream is( _name.c_str(), std::ios::in);
    if (is.is_open()) {

        glm::vec3 position;
        cameraPath.clear();

        std::string line;
        // glm::mat4 rot = glm::mat4(
        //         1.0f,   0.0f,   0.0f,   0.0f,
        //         0.0f,  1.0f,    0.0f,   0.0f,
        //         0.0f,   0.0f,   -1.0f,   0.0f,
        //         0.0f,   0.0f,   0.0f,   1.0f
        //     );

        while (std::getline(is, line)) {
            // If line not commented 
            if (line[0] == '#')
                continue;

            // parse through row spliting into commas
            std::vector<std::string> params = vera::split(line, ',', true);

            CameraData frame;
            float fL = vera::toFloat(params[0]);
            float cx = vera::toFloat(params[1]);
            float cy = vera::toFloat(params[2]);

            float near = 0.0f;
            float far = 1000.0;

            if (activeCamera) {
                near = activeCamera->getNearClip();
                far = activeCamera->getFarClip();
            }

            float delta = far-near;
            float w = cx*2.0f;
            float h = cy*2.0f;

            // glm::mat4 projection = glm::ortho(0.0f, w, h, 0.0f, near, far);
            // glm::mat4 ndc = glm::mat4(
            //     fL,     0.0f,   0.0f,   0.0f,
            //     0.0f,   fL,     0.0f,   0.0f, 
            //     cx,     cy,     1.0f,   0.0f,
            //     0.0f,   0.0f,   0.0f,   1.0f  
            // );
            // frame.projection = projection * ndc;

            frame.projection = glm::mat4(
                2.0f*fL/w,          0.0f,               0.0f,                       0.0f,
                0.0f,               -2.0f*fL/h,         0.0f,                       0.0f,
                (w - 2.0f*cx)/w,    (h-2.0f*cy)/h,      (-far-near)/delta,         -1.0f,
                0.0f,               0.0f,               -2.0f*far*near/delta,       0.0f
            );
            
            // frame.projection = glm::mat4(
            //     fL/cx,      0.0f,   0.0f,                   0.0f,
            //     0.0f,       fL/cy,  0.0f,                   0.0f,
            //     0.0f,       0.0f,   -(far+near)/delta,      -1.0,
            //     0.0f,       0.0f,   -2.0*far*near/delta,    0.0f
            // );

            frame.transform = glm::mat4(
                glm::vec4( vera::toFloat(params[3]), vera::toFloat(params[ 4]), vera::toFloat(params[ 5]), 0.0),
                glm::vec4( vera::toFloat(params[6]), -vera::toFloat(params[ 7]), vera::toFloat(params[ 8]), 0.0),
                glm::vec4( vera::toFloat(params[9]), vera::toFloat(params[10]), vera::toFloat(params[11]), 0.0),
                glm::vec4( vera::toFloat(params[12]), vera::toFloat(params[13]), -vera::toFloat(params[14]), 1.0)
            );

            // position += frame.translation;
            cameraPath.push_back(frame);
        }

        std::cout << "// Added " << cameraPath.size() << " camera frames" << std::endl;

        position = position / float(cameraPath.size());
        return true;
    }

    return false;
}
