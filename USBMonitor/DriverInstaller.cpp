#include "StdAfx.h"
#include "DriverInstaller.h"
#include "DriverInstallerThread.h"
#include "App.h"

DriverInstaller::DriverInstaller(DriverInstallerCallback *pCallback)
	: m_bIsRunning(false)
	, m_pCallback(pCallback)
	, m_type(DPINST)
{
	m_pThread = new DriverInstallerThread(this);
	ASSERT(m_pThread);
	m_cs.Init();
}

DriverInstaller::~DriverInstaller(void)
{
	delete m_pThread;
}

static CString GetEncodedFilePath(const CString& strPath)
{
	CString result = strPath;
	result.Replace(_T(" "), _T("\" \""));
	return result;
}

void DriverInstaller::Start(InstallType type, const CString& path)
{
	if (IsRunning())
	{
		return;
	}
	m_type = type;
	CString file;
	CString params;
	bool showWindow = false;
	switch(m_type)
	{
	case DPINST:
		{
			file = CPaintManagerUI::GetInstancePath() + (Is64BitWindows() ? _T("dpinst64.exe") : _T("dpinst32.exe"));
			CString driverPath = CPaintManagerUI::GetInstancePath() + (LPCTSTR)path;
			params.Format(_T(" /Q /SH /C /PATH %s"), (LPCTSTR)GetEncodedFilePath(driverPath));
			showWindow = false;
		}
		break;
	case EXE:
		{
			file = path;
			showWindow = true;
		}
		break;
	}
	m_pThread->SetFile(file);
	m_pThread->SetEncodedParameters(params);
	m_pThread->SetShowWindow(showWindow);
	m_pThread->Start();
	m_pThread->Event();
}

void DriverInstaller::Abort()
{
	if (!IsRunning())
	{
		return;
	}
	m_pThread->Stop();
}

bool DriverInstaller::IsRunning()
{
	m_cs.Lock();
	bool ret = m_bIsRunning;
	m_cs.Unlock();
	return ret;
}

void DriverInstaller::OnThreadTerminated(bool success)
{
	TRACE(_T("DriverInstaller::OnThreadTerminated\nError Code: %x\nError Message:\n\n"),
		m_pThread->GetExitCode(),
		m_pThread->GetErrorMessge());
	switch(m_type)
	{
	case DPINST:
		break;
	case EXE:
		break;
	}
	m_pCallback->OnDriverInstalled(success);
	m_cs.Lock();
	m_bIsRunning = false;
	m_cs.Unlock();
}