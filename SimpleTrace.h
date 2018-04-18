/*
 *  \file   SimpleTrace.h
 *  \date   Copyright (c) 2015, Arm Limited or its affiliates. All rights reserved.
 */

#include "MTI/PluginInterface.h"
#include "MTI/PluginFactory.h"
#include "MTI/PluginInstance.h"
#include "MTI/ModelTraceInterface.h"
#include "eslapi/CAInterfaceRegistry.h"
#include "sg/SGComponentRegistry.h"

#include <list>
#include <string>
#include <algorithm>
#include <cstdio>

#ifdef SG_MODEL_BUILD
    #include "builddata.h"
    #define PLUGIN_VERSION FULL_VERSION_STRING
#else
    #define PLUGIN_VERSION "unreleased"
#endif

/////////////////////////////////////////////////////////////////////////////

class SimpleTrace :
    public MTI::PluginInstance
{
public:
    virtual eslapi::CAInterface * ObtainInterface(eslapi::if_name_t    ifName,
                                                  eslapi::if_rev_t     minRev,
                                                  eslapi::if_rev_t *   actualRev);

    SimpleTrace(const char *instance_name,
                       uint32_t num_parameters,
                       eslapi::CADIParameterValue_t *parameter_values);


    /** This is to associate a plugin with a simulation instance. Exactly one simulation must be registered. */
    virtual eslapi::CADIReturn_t RegisterSimulation(eslapi::CAInterface *simulation);

    // This is called before the plugin .dll/.so is unloaded and should allow the plugin to do it's cleanup.
    virtual void Release();

    virtual const char *GetName() const;

    int fifo_init();
	int fifo_write(uint32_t pc);

public: // methods
    MTI::EventClass *event_class;

    static void
    TracePC_Thunk(void * user_data,
                  const MTI::EventClass *event_class,
                  const struct MTI::EventRecord *record);

    eslapi::CADIReturn_t Error(const char *message) const;

private:
    // Interface registry for a convenient implementation of ObtainInterface().
    eslapi::CAInterfaceRegistry interfaceRegistry;
    std::string instance_name;
    MTI::ValueIndex inst_pc_index;

    void
    TracePC(const MTI::EventClass *event_class,
            const struct MTI::EventRecord *record);

};

class ThePluginFactory :
    public MTI::PluginFactory
{
public:
    ThePluginFactory() : interfaceRegistry("ThePluginFactory")
    {
        interfaceRegistry.Register<MTI::PluginFactory>(this);
    }

    virtual eslapi::CAInterface * ObtainInterface(eslapi::if_name_t    ifName,
                                                  eslapi::if_rev_t     minRev,
                                                  eslapi::if_rev_t *   actualRev);

    virtual uint32_t GetNumberOfParameters();

    virtual eslapi::CADIReturn_t GetParameterInfos(eslapi::CADIParameterInfo_t *parameter_info_list);

    virtual eslapi::CAInterface *Instantiate(const char *instance_name,
                                             uint32_t number_of_parameters,
                                             eslapi::CADIParameterValue_t *parameter_values);

    virtual void Release();

    virtual const char *GetType() const { return "SimpleTrace"; }
    virtual const char *GetVersion() const { return PLUGIN_VERSION; }

private:
    // Interface registry for a convenient implementation of ObtainInterface().
    eslapi::CAInterfaceRegistry interfaceRegistry;
};
