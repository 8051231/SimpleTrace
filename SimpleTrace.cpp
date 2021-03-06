﻿/*!
 *  \file   SimpleTrace.cpp
 *  \brief  Very simple trace example, tracing the PC
 *  \date   Copyright ARM Limited 2009 All Rights Reserved.
 *
 */

#include "SimpleTrace.h"
#include <Windows.h>

#include <tchar.h>
#include <winsock.h>

const char *pStrPipeName = "\\\\.\\pipe\\Name_pipe_demon_get";
const int BUFFER_MAX_LEN = 1024;

HANDLE hPipe = NULL;

void print_data_syslog(char *msg, void * data, unsigned short len)
{
	if (msg == NULL || data == NULL || len < 1)
	{
		return;
	}
	unsigned char * pbuf = (unsigned char*)data;

		int i = 0;
		int j = 0;

		char tmp_buff[1024] = { 0 };

		for (j = 0; j < len;)
		{
			memset(tmp_buff, 0, sizeof(tmp_buff));

			for (i = 0; i < sizeof(tmp_buff) - 3 && j < len; j++)
			{
				sprintf(tmp_buff + i, "%02x ", pbuf[j]);
				i = strlen(tmp_buff);
			}

			printf("%s%s\n", msg,tmp_buff);
		}

}

void  PrintHex(char *msg, void * data, unsigned short len)
{
	print_data_syslog(msg, data, len);
}

int SimpleTrace::fifo_recv(uint32_t pc)
{
	uint32_t nRetPC = 0;
	int ret = 0;
	DWORD nRcvLen = 0;
	unsigned char sRcvbuf[4] = { 0 };
	nRcvLen = 0;
	BOOL fSuccess = FALSE;

	printf("client wait for server pipe response---------------\n");
	fSuccess = ReadFile(hPipe, sRcvbuf, 4, &nRcvLen, NULL);

	if (!fSuccess || nRcvLen == 0)
	{
		if (GetLastError() == ERROR_BROKEN_PIPE)
		{
			_tprintf(TEXT("client disconnected.\n"), GetLastError());
		}
		else
		{
			_tprintf(TEXT("client ReadFile failed, GLE=%d.\n"), GetLastError());
		}
		return -1;
	}

	PrintHex("Client_rcv: ", sRcvbuf, nRcvLen);
	nRetPC = (sRcvbuf[3] << 24) | (sRcvbuf[2] << 16) | (sRcvbuf[1] << 8) | (sRcvbuf[0]);
	printf("client receive data from the server pipe %d bytes,contents are:0x%08x\n", nRcvLen, nRetPC);
	//Sleep(100);

	if (pc == nRetPC)
	{

		printf("client send->server rcv->server->send->client rcv success\n");
	}
	else
	{

		printf("client send->server rcv->server send->client rcv failed\n");
		return -1;
	}
	printf("client recieve data from server pipe success\n");
	return 0;
}

int SimpleTrace::fifo_write(uint32_t pc)
{

	DWORD nSendLen = 0;
	unsigned char pbuf[4] = { 0 };
	BOOL fSuccess = FALSE;

	memcpy(&pbuf[0], &pc, 4);

	//向服务端发送数据  
	printf("client write data to server pipe begin\n");

	fSuccess = WriteFile(hPipe, pbuf, 4, &nSendLen, NULL);
    if (!fSuccess || nSendLen == 0)
	{
		if (GetLastError() == ERROR_BROKEN_PIPE)
		{
			_tprintf(TEXT("client disconnected.\n"), GetLastError());
		}
		else
		{
			_tprintf(TEXT("client ReadFile failed, GLE=%d.\n"), GetLastError());
		}
		return -1;
	}
	else
	{
		printf("client write data to server pipe  %d bytes, 0x%08x\n", nSendLen, pc);
	}

	PrintHex("Client_send: ", pbuf, nSendLen);

    //FlushFileBuffers(hPipe);
	printf("client write data to server success finished\n");
	//Sleep(1000);
	return 0;
}
int SimpleTrace::fifo_init()
{
	printf("plug-in begins to wait...\n");
	if (!WaitNamedPipe(pStrPipeName, NMPWAIT_WAIT_FOREVER))
	{
		printf("client Error! Connection to Get failure\n");
		return 0;
	}
	printf("client CreateFile begin\n");
	hPipe = CreateFile(pStrPipeName, GENERIC_READ | GENERIC_WRITE, 0,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	printf("client CreateFile finish\n");

	return 0;
}

SimpleTrace::SimpleTrace(const char *instance_name_,
                                       uint32_t /*number_parameters*/,
                                       eslapi::CADIParameterValue_t * /*parameter_values*/) :
    instance_name(instance_name_),
    inst_pc_index(-1),
    interfaceRegistry("SimpleTrace")
{
	printf("-------------------entry SimpleTrace------------\n");

    interfaceRegistry.Register<PluginInstance>(this);
	//

	printf("-------------------begin fifo_init SimpleTrace------------\n");

	//初始化管道
	fifo_init();

	printf("-------------------end fifo_init SimpleTrace------------\n");

}

eslapi::CAInterface *
SimpleTrace::ObtainInterface(eslapi::if_name_t    ifName,
                                    eslapi::if_rev_t     minRev,
                                    eslapi::if_rev_t *   actualRev)
{
    return interfaceRegistry.ObtainInterface(ifName, minRev, actualRev);
}

eslapi::CADIReturn_t
SimpleTrace::Error(const char *message) const
{
    fprintf(stderr, "Error: Trace plugin %s: %s\n", instance_name.c_str(), message);
    return eslapi::CADI_STATUS_GeneralError;
}

eslapi::CADIReturn_t
SimpleTrace::RegisterSimulation(eslapi::CAInterface *ca_interface)
{
    if (!ca_interface)
        return eslapi::CADI_STATUS_IllegalArgument;
	printf("entry RegisterSimulation\n");
    // Get the System Trace Interface:
    MTI::SystemTraceInterface *sti = ca_interface->ObtainPointer<MTI::SystemTraceInterface>();

    if (!sti)
        return eslapi::CADI_STATUS_IllegalArgument;

    // Check if there is at least one component with trace:
    MTI::SystemTraceInterface::TraceComponentIndex num_trace_components = sti->GetNumOfTraceComponents();
    if (num_trace_components == 0)
        return Error("No components provide trace.");


    // For each component get the Trace Component Interface and see if it has an INST source
    for(MTI::SystemTraceInterface::TraceComponentIndex tci=0; tci < num_trace_components; ++tci)
    {
        eslapi::CAInterface *caif = sti->GetComponentTrace(tci);
        MTI::ComponentTraceInterface *cti = caif->ObtainPointer<MTI::ComponentTraceInterface>();
        if(cti)
        {
            // Get the trace source named "INST":
            MTI::TraceSource *source = cti->GetTraceSource("INST");
            if (source)
            {
                printf("Attached %s to component: %s\n", instance_name.c_str(), sti->GetComponentTracePath(tci));

                // Now find the field "PC", and create a field mask:
                const MTI::EventFieldType *field = source->GetField("PC");
                if (!field)
                    return Error("No field named \"PC\" found in \"INST\" trace source.");
                MTI::FieldMask mask = 1 << field->GetIndex();

                // Register an event class:
                event_class = source->CreateEventClass(mask);
                if (!event_class)
                    return Error("Problem when creating EventClass.");

                // Now register the callback:
                MTI::Status status = event_class->RegisterCallback(TracePC_Thunk, this);
                if (status != MTI::MTI_OK)
                    return Error("EventClass::RegisterCallback() returned error.");

                inst_pc_index = event_class->GetValueIndex("PC");
                if (inst_pc_index == -1)
                    return Error("EventClass::GetValueIndex(\"PC\") returned error.");
            }
            else
            {
                printf("Ignoring component %s as it does not contain an INST source\n", sti->GetComponentTracePath(tci));
            }
        }
    }
    return eslapi::CADI_STATUS_OK;
}

// This is called before the plugin .dll/.so is unloaded and should allow the plugin to do it's cleanup.
void
SimpleTrace::Release()
{
	//add by cxl begin
	if (NULL != hPipe)
	{
		CloseHandle(hPipe);
		hPipe = NULL;
	}
	//add by cxl end
    delete this;
}

const char *
SimpleTrace::GetName() const
{
    return instance_name.c_str();
}

/////////////////////////////////////////////////////////////////////////////

void SimpleTrace::TracePC(const MTI::EventClass *event_class,
                                 const struct MTI::EventRecord *record)
{
    uint32_t pc = record->Get<uint32_t>(event_class, inst_pc_index);


    printf("SimpleTrace------------PC: 0x%08x\n", pc);

	//add by cxl begin
	//FILE *fp;
	//fp = fopen("D:\\a.txt", "a+");
	//if (fp == 0) { printf("can't open file\n"); }
	//fprintf(fp, "%08x   ", pc);  //字符使用%c

	//将pc值通过管道发送给服务端
	int nRet = 0;
	nRet = fifo_write(pc);
	if (nRet != 0)
	{
		printf("client fifo_write to server pipe failed, exit\n");
		if (NULL != hPipe)
		{
			CloseHandle(hPipe);
			hPipe = NULL;
		}
		
		exit(1);
	}
	//fprintf(fp, "EXIT\n", pc);  //字符使用%c
    //fclose(fp);
	//等待回馈

	nRet = fifo_recv(pc);
	if (nRet != 0)
	{
		printf("client fifo_recv from server pipe failed, exit\n");
		if (NULL != hPipe)
		{
			CloseHandle(hPipe);
			hPipe = NULL;
		}
		exit(1);
	}
    //	Sleep(1000);
	//add by cxl end  
}

// The callback interface is a C interface, so we need a short thunk
// function to call into a C++ method. This is done via a static C++
// method, which behaves like a C function:
void
SimpleTrace::TracePC_Thunk(void * user_data,
                                  const MTI::EventClass *event_class,
                                  const struct MTI::EventRecord *record)
{
    SimpleTrace *simple_trace = reinterpret_cast<SimpleTrace *>(user_data);
    simple_trace->TracePC(event_class, record);
}

/////////////////////////////////////////////////////////////////////////////



eslapi::CAInterface *
ThePluginFactory::ObtainInterface(eslapi::if_name_t    ifName,
                                  eslapi::if_rev_t     minRev,
                                  eslapi::if_rev_t *   actualRev)
{
    return interfaceRegistry.ObtainInterface(ifName, minRev, actualRev);
}

uint32_t
ThePluginFactory::GetNumberOfParameters()
{
    return 0;
}


eslapi::CADIReturn_t
ThePluginFactory::GetParameterInfos(eslapi::CADIParameterInfo_t* /*parameter_info_list*/)
{
    return eslapi::CADI_STATUS_CmdNotSupported;
}


eslapi::CAInterface *
ThePluginFactory::Instantiate(const char *instance_name,
                              uint32_t    number_of_parameters,
                              eslapi::CADIParameterValue_t *param_values)
{
    return static_cast<MTI::PluginInstance *>(new SimpleTrace(instance_name, number_of_parameters, param_values));
}

void
ThePluginFactory::Release()
{
    // nothing to do since factory is a static instance
}

static ThePluginFactory factory_instance;

extern "C"
{
    eslapi::CAInterface *
    GetCAInterface()
    {
		printf("entry GetCAInterface\n");
        return &factory_instance;
    }
}
// End of file SimpleTrace.cpp
