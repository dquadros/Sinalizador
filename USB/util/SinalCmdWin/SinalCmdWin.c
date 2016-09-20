// Exemplo simples de controle do sinalizador ligado à USB
// Daniel Quadros - 11/09/16 - https://dqsoft.blogpsot.com
//
// Colocar na linha de comando os LEDs que devem ficar acesos
// R (red = vermelho), O (orange = laranja), G (green = verde), W (white = branco)
// Ex: "SinalCmdWin RG" acende os LEDS vermelho e verde e apaga os demais

#include <stdio.h>
#include <windows.h>
#include <setupapi.h>

// Identificação do nosso dispositivo
#define MEU_VID	0x16C0
#define MEU_PID	0x05DF

// Definição das rotinas SetupDI utilizadas
typedef HDEVINFO (WINAPI* SETUPDIGETCLASSDEVS) (CONST GUID*, PCSTR, HWND, DWORD);
typedef BOOL (WINAPI* SETUPDIENUMDEVICEINTERFACES) (HDEVINFO, PSP_DEVINFO_DATA, CONST GUID*, DWORD, PSP_DEVICE_INTERFACE_DATA);
typedef BOOL (WINAPI* SETUPDIGETDEVICEINTERFACEDETAIL) (HDEVINFO, PSP_DEVICE_INTERFACE_DATA, PSP_DEVICE_INTERFACE_DETAIL_DATA_A,
														DWORD, PDWORD, PSP_DEVINFO_DATA);
typedef BOOL (WINAPI* SETUPDIDESTROYDEVICEINFOLIST) (HDEVINFO);	

// Ponteiros para as rotinas SetupDI
SETUPDIGETCLASSDEVS pSetupDiGetClassDevs = NULL;
SETUPDIENUMDEVICEINTERFACES pSetupDiEnumDeviceInterfaces = NULL;
SETUPDIGETDEVICEINTERFACEDETAIL pSetupDiGetDeviceInterfaceDetail = NULL;
SETUPDIDESTROYDEVICEINFOLIST pSetupDiDestroyDeviceInfoList = NULL;

// Estrutura HIDD_ATTIBUTES
typedef struct _HIDD_ATTRIBUTES {
    ULONG   Size;
    USHORT  VendorID;
    USHORT  ProductID;
    USHORT  VersionNumber;
} HIDD_ATTRIBUTES, *PHIDD_ATTRIBUTES;

// Definição das rotinas HidD utilizadas
typedef void (__stdcall*GETHIDGUID) (OUT LPGUID HidGuid);
typedef BOOLEAN (__stdcall*GETATTRIBUTES)(IN HANDLE HidDeviceObject,OUT PHIDD_ATTRIBUTES Attributes);
typedef BOOLEAN (__stdcall*SETNUMINPUTBUFFERS)(IN  HANDLE HidDeviceObject,OUT ULONG  NumberBuffers);
typedef BOOLEAN (__stdcall*GETNUMINPUTBUFFERS)(IN  HANDLE HidDeviceObject,OUT PULONG  NumberBuffers);
typedef BOOLEAN (__stdcall*GETFEATURE) (IN  HANDLE HidDeviceObject, OUT PVOID ReportBuffer, IN ULONG ReportBufferLength);
typedef BOOLEAN (__stdcall*SETFEATURE) (IN  HANDLE HidDeviceObject, IN PVOID ReportBuffer, IN ULONG ReportBufferLength);
typedef BOOLEAN (__stdcall*GETREPORT) (IN  HANDLE HidDeviceObject, OUT PVOID ReportBuffer, IN ULONG ReportBufferLength);
typedef BOOLEAN (__stdcall*SETREPORT) (IN  HANDLE HidDeviceObject, IN PVOID ReportBuffer, IN ULONG ReportBufferLength);
typedef BOOLEAN (__stdcall*GETMANUFACTURERSTRING) (IN  HANDLE HidDeviceObject, OUT PVOID ReportBuffer, IN ULONG ReportBufferLength);
typedef BOOLEAN (__stdcall*GETPRODUCTSTRING) (IN  HANDLE HidDeviceObject, OUT PVOID ReportBuffer, IN ULONG ReportBufferLength);
typedef BOOLEAN (__stdcall*GETINDEXEDSTRING) (IN  HANDLE HidDeviceObject, IN ULONG  StringIndex, OUT PVOID ReportBuffer, IN ULONG ReportBufferLength);

// Ponteiros para as rotinas HidD
GETHIDGUID HidD_GetHidGuid = NULL;
GETATTRIBUTES HidD_GetAttributes = NULL;
SETNUMINPUTBUFFERS HidD_SetNumInputBuffers = NULL;
GETNUMINPUTBUFFERS HidD_GetNumInputBuffers = NULL;
GETFEATURE HidD_GetFeature = NULL;
SETFEATURE HidD_SetFeature = NULL;
GETREPORT HidD_GetInputReport = NULL;
SETREPORT HidD_SetOutputReport = NULL;
GETMANUFACTURERSTRING HidD_GetManufacturerString = NULL;
GETPRODUCTSTRING HidD_GetProductString = NULL;

// Nome completo do dispositivo
char DevPathName[_MAX_PATH];

// Rotinas locais
int CarregaSetupApi ();
int CarregaHID ();
HANDLE BuscaDisp ();

// Programa principal
int main (int argc, char *argv[]) {
	HANDLE hDisp;
	BYTE leds;
	BYTE buf[4];

	// Carrega dinamicamente as bibliotecas
	if (!CarregaSetupApi()) {
		return 1;
	}
	if (!CarregaHID()) {
		return 2;
	}

	// Procura o nosso dispositivo
	hDisp = BuscaDisp();
	if (hDisp == INVALID_HANDLE_VALUE) {
		return 3;
	}

	// Programa o novo valor dos LEDs
	leds = 0;
	if (argc > 1) {
		char *p = argv[1];
		while (*p) {
			switch (*p) {
				case 'r': case 'R':	leds |= 0x08; break;
				case 'o': case 'O':	leds |= 0x04; break;
				case 'g': case 'G':	leds |= 0x20; break;
				case 'w': case 'W':	leds |= 0x10; break;
			}
			p++;
		}
	}
	buf[0] = 0;		// indica que só temos um tipo de report
	buf[1] = 1;		// identificação do dispositivo
	buf[2] = 0;		// 
	buf[3] = leds;	// programação dos leds
	if (!HidD_SetFeature (hDisp, buf, 4)) {
		printf ("Erro %d ao atualizar o sinalizador\n", GetLastError());
	}
	CloseHandle (hDisp);

	return 0;
}


// Carrega SetupApi
// Retorna true se sucesso
int CarregaSetupApi () {
	HMODULE hSAPI = NULL;

	// Tenta carregar dinamicamente a biblioteca SetupApi
	hSAPI = LoadLibrary("setupapi.dll");
	if(hSAPI == NULL){ 
		printf("Nao achou setupapi.dll\n");
		return FALSE;
	}

	// Pega o endereço das rotinas
	pSetupDiGetClassDevs = (SETUPDIGETCLASSDEVS) GetProcAddress(hSAPI, "SetupDiGetClassDevsA");
	pSetupDiEnumDeviceInterfaces = (SETUPDIENUMDEVICEINTERFACES) GetProcAddress(hSAPI, "SetupDiEnumDeviceInterfaces");
	pSetupDiGetDeviceInterfaceDetail = (SETUPDIGETDEVICEINTERFACEDETAIL) GetProcAddress(hSAPI, "SetupDiGetDeviceInterfaceDetailA");
	pSetupDiDestroyDeviceInfoList = (SETUPDIDESTROYDEVICEINFOLIST) GetProcAddress(hSAPI, "SetupDiDestroyDeviceInfoList");

	// Confere se teve sucesso
	if((pSetupDiGetClassDevs == NULL) ||
	   (pSetupDiEnumDeviceInterfaces == NULL) ||
	   (pSetupDiDestroyDeviceInfoList == NULL) ||
	   (pSetupDiGetDeviceInterfaceDetail == NULL)) {
		printf("Nao encontrou as funcoes necessarias em setupapi.dll\n");
		return FALSE;
	}

	return TRUE;
}

// Carrega HID.dll
// Retorna true se sucesso
int CarregaHID () {
	HMODULE hHID = NULL;

	// Tenta carregar dinamicamente a biblioteca SetupApi
	hHID = LoadLibrary("hid.dll");
	if(hHID == NULL){ 
		printf("Nao achou hid.dll\n");
		return FALSE;
	}

	// Pega o endereço das rotinas
	HidD_GetHidGuid = (GETHIDGUID)GetProcAddress(hHID, "HidD_GetHidGuid");
	HidD_GetAttributes = (GETATTRIBUTES)GetProcAddress(hHID, "HidD_GetAttributes");
	HidD_SetNumInputBuffers = (SETNUMINPUTBUFFERS)GetProcAddress(hHID, "HidD_SetNumInputBuffers");
	HidD_GetNumInputBuffers = (GETNUMINPUTBUFFERS)GetProcAddress(hHID, "HidD_GetNumInputBuffers");
	HidD_GetFeature = (GETFEATURE)GetProcAddress(hHID, "HidD_GetFeature");
	HidD_SetFeature = (SETFEATURE)GetProcAddress(hHID, "HidD_SetFeature");
	HidD_GetInputReport = (GETREPORT)GetProcAddress(hHID, "HidD_GetInputReport");
	HidD_SetOutputReport = (SETREPORT)GetProcAddress(hHID, "HidD_SetOutputReport");
	HidD_GetManufacturerString = (GETMANUFACTURERSTRING)GetProcAddress(hHID, "HidD_GetManufacturerString");
	HidD_GetProductString = (GETPRODUCTSTRING)GetProcAddress(hHID, "HidD_GetProductString");

	// Confere se teve sucesso
	if((HidD_GetHidGuid == NULL) ||
		(HidD_GetAttributes == NULL) ||
		(HidD_SetNumInputBuffers == NULL) ||
		(HidD_GetNumInputBuffers == NULL) ||
		(HidD_GetFeature == NULL) ||
		(HidD_SetFeature == NULL) ||
		(HidD_GetInputReport == NULL) ||
		(HidD_SetOutputReport == NULL) ||
		(HidD_GetManufacturerString == NULL) ||
		(HidD_GetProductString == NULL)) {
		printf("Nao encontrou as funcoes necessarias em hid.dll\n");
		return FALSE;
	}

	return TRUE;
}


// Procura o nosso dispositivo
// Retorna handle se tiver sucesso, INVALID_HANDLE_VALUE se ocorrer erro
HANDLE BuscaDisp () {
	GUID HidGuid;
	HANDLE hDevInfo;
	SP_DEVICE_INTERFACE_DATA devInfoData;
	int devIndex;
	int achou;
	DWORD Length;
	DWORD dummy;
	PSP_DEVICE_INTERFACE_DETAIL_DATA detailData;
	HANDLE DeviceHandle;
	HIDD_ATTRIBUTES Attributes;
	wchar_t string[128];

	// Obtem o GUID da classe HID
	HidD_GetHidGuid(&HidGuid);

	// Obtem a lista de dispositivos HID
	hDevInfo = pSetupDiGetClassDevs (&HidGuid, NULL, NULL, DIGCF_PRESENT|DIGCF_INTERFACEDEVICE);

	// Procurar o nosso dispositivo na lista
	achou = FALSE;
	for (devIndex = 0; !achou; devIndex++) {
		// Examinar o próximo dispositivo
		devInfoData.cbSize = sizeof(devInfoData);
		if (pSetupDiEnumDeviceInterfaces (hDevInfo, 0, &HidGuid, devIndex, &devInfoData) == 0) {
			// Fim dos dispositivos, não encontramos
			break;
		}

		// Pegar os detalhes
		pSetupDiGetDeviceInterfaceDetail (hDevInfo, &devInfoData, NULL, 0, &Length, NULL);	// obtem tamanho
		detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) malloc(Length);
		if (detailData == NULL) {
			// Faltou memoria
			break;
		}
		detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		pSetupDiGetDeviceInterfaceDetail (hDevInfo, &devInfoData, detailData, Length, &dummy, NULL);

		// Obtem um handle para o dispositivo
		DeviceHandle = CreateFile (detailData->DevicePath,
					0, FILE_SHARE_READ|FILE_SHARE_WRITE,
					(LPSECURITY_ATTRIBUTES)NULL, OPEN_EXISTING, 0, NULL);
		if (DeviceHandle == INVALID_HANDLE_VALUE) {
			// Não deveria acontecer... ignora o dispositivo
			printf ("Ignorando %s\n", detailData->DevicePath);
			free (detailData);
			continue;
		}

		// Vejamos se é o nosso
		Attributes.Size = sizeof(Attributes);
		HidD_GetAttributes (DeviceHandle, &Attributes);
		if ((Attributes.VendorID == MEU_VID) && (Attributes.ProductID == MEU_PID)) {
			achou = TRUE;
			strcpy (DevPathName, detailData->DevicePath);
		}

		// Fecha o handle e libera a area com os detalhes
		CloseHandle (DeviceHandle);
		free (detailData);
	}

	// Libera a lista
	pSetupDiDestroyDeviceInfoList(hDevInfo);

	if (!achou) {
		printf ("Nao achou dispositivo\n");
		return INVALID_HANDLE_VALUE;
	}

	// Obter um novo handle
	DeviceHandle = CreateFile (DevPathName,
						GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
						(LPSECURITY_ATTRIBUTES)NULL, OPEN_EXISTING, 0, NULL);

	// Listar algumas informações
	printf ("Dispositivo achado em\n%s\n", DevPathName);
	if (HidD_GetManufacturerString (DeviceHandle, string, sizeof(string))) {
		wprintf(L"Fabricante %s\n",string);
	}
	if (HidD_GetProductString (DeviceHandle, string, sizeof(string))) {
		wprintf(L"Produto: %s\n",string);
	}

	return DeviceHandle;
}

