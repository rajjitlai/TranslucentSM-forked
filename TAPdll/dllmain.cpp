// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Controls.Primitives.h>

using namespace winrt;
using namespace Windows::Foundation;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::UI;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Controls;
using namespace winrt::Windows::UI::Xaml::Media;

DWORD dwRes = 0, dwSize = sizeof(DWORD), dwOpacity = 0, dwLuminosity = 0;
typedef void (WINAPI* RtlGetVersion_FUNC) (OSVERSIONINFOEXW*);
OSVERSIONINFOEX os;

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
template <typename T>
T convert_from_abi(com_ptr<::IInspectable> from)
{
	T to{ nullptr }; // `T` is a projected type.

	winrt::check_hresult(from->QueryInterface(winrt::guid_of<T>(),
		winrt::put_abi(to)));

	return to;
}

DependencyObject FindDescendantByName(DependencyObject root, hstring name)
{
	if (root == nullptr)
	{
		return nullptr;
	}

	int count = VisualTreeHelper::GetChildrenCount(root);
	for (int i = 0; i < count; i++)
	{
		DependencyObject child = VisualTreeHelper::GetChild(root, i);
		if (child == nullptr)
		{
			continue;
		}

		hstring childName = child.GetValue(FrameworkElement::NameProperty()).as<hstring>();
		if (childName == name)
		{
			return child;
		}

		DependencyObject result = FindDescendantByName(child, name);
		if (result != nullptr)
		{
			return result;
		}
	}

	return nullptr;
}

struct ExplorerTAP : winrt::implements<ExplorerTAP, IObjectWithSite>
{
	HRESULT STDMETHODCALLTYPE SetSite(IUnknown* pUnkSite) noexcept override
	{
		site.copy_from(pUnkSite);
		com_ptr<IXamlDiagnostics> diag = site.as<IXamlDiagnostics>();
		com_ptr<::IInspectable> dispatcherPtr;
		diag->GetDispatcher(dispatcherPtr.put());
		CoreDispatcher dispatcher = convert_from_abi<CoreDispatcher>(dispatcherPtr);
		RegGetValue(HKEY_CURRENT_USER, L"Software\\TranslucentSM", L"TintOpacity", RRF_RT_DWORD, NULL, &dwOpacity, &dwSize);
		RegGetValue(HKEY_CURRENT_USER, L"Software\\TranslucentSM", L"TintLuminosityOpacity", RRF_RT_DWORD, NULL, &dwLuminosity, &dwSize);

		dispatcher.RunAsync(CoreDispatcherPriority::Normal, []()
			{
				auto content = Window::Current().Content();
				// Search for AcrylicBorder name
				static auto acrylicBorder = FindDescendantByName(content, L"AcrylicBorder").as<Border>();
				if (acrylicBorder != nullptr)
				{
					if (dwOpacity > 10) dwOpacity = 10;
					if (dwLuminosity > 10) dwLuminosity = 10;
					acrylicBorder.Background().as<AcrylicBrush>().TintOpacity(double(dwOpacity) / 10);
					acrylicBorder.Background().as<AcrylicBrush>().TintLuminosityOpacity(double(dwLuminosity) / 10);
				}
				auto nvpane = FindDescendantByName(content, L"NavigationPanePlacesListView");
				auto rootpanel = FindDescendantByName(nvpane, L"Root");
				auto grid = VisualTreeHelper::GetChild(rootpanel, 0).as<Grid>();
				Button bt;
				auto f = FontIcon();
				f.Glyph(L"\uE104");
				f.FontFamily(Media::FontFamily(L"Segoe Fluent Icons"));
				bt.Content(winrt::box_value(f));
				Thickness buttonMargin;
				buttonMargin.Left = -40;    
				buttonMargin.Top = 0;     
				buttonMargin.Right = 0;
				buttonMargin.Bottom = 0;  
				bt.Margin(buttonMargin);
				Thickness buttonPad;
				buttonPad.Left = 8.2;
				buttonPad.Right = 8.2;
				buttonPad.Top = 8;
				buttonPad.Bottom = 8;
				bt.Padding(buttonPad);
				bt.Width(38);
				bt.FontFamily(Windows::UI::Xaml::Media::FontFamily(L"Segoe UI Semibold"));
				grid.Children().Append(bt);

				auto stackPanel = StackPanel();

				Slider slider;
				slider.Width(100);
				slider.Value(dwOpacity * 10);
				stackPanel.Children().Append(slider);

				Slider slider2;
				slider2.Width(100);
				slider2.Value(dwLuminosity * 10);
				stackPanel.Children().Append(slider2);
				
				slider.ValueChanged([](IInspectable const& sender, RoutedEventArgs const&) {
					auto sliderControl = sender.as<Slider>();
					double sliderValue = sliderControl.Value();
					acrylicBorder.Background().as<AcrylicBrush>().TintOpacity(double(sliderValue) / 100);
					});

				slider2.ValueChanged([](IInspectable const& sender, RoutedEventArgs const&) {
					auto sliderControl = sender.as<Slider>();
					double sliderValue = sliderControl.Value();
					acrylicBorder.Background().as<AcrylicBrush>().TintLuminosityOpacity(double(sliderValue) / 100);
					});

				Flyout flyout;
				flyout.Content(stackPanel);
				bt.Flyout(flyout);
			});
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, void** ppvSite) noexcept override
	{
		return site.as(riid, ppvSite);
	}

private:
	winrt::com_ptr<IUnknown> site;
};
struct TAPFactory : winrt::implements<TAPFactory, IClassFactory>
{
	HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject) override try
	{
		*ppvObject = nullptr;

		if (pUnkOuter)
		{
			return CLASS_E_NOAGGREGATION;
		}

		return winrt::make<ExplorerTAP>().as(riid, ppvObject);
	}
	catch (...)
	{
		return winrt::to_hresult();
	}

	HRESULT STDMETHODCALLTYPE LockServer(BOOL) noexcept override
	{
		return S_OK;
	}
};
_Use_decl_annotations_ STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) try
{
	*ppv = nullptr;

	// TODO: move this somewhere common
	// {36162BD3-3531-4131-9B8B-7FB1A991EF51}
	static constexpr GUID temp =
	{ 0x36162bd3, 0x3531, 0x4131, { 0x9b, 0x8b, 0x7f, 0xb1, 0xa9, 0x91, 0xef, 0x51 } };
	if (rclsid == temp)
	{
		return winrt::make<TAPFactory>().as(riid, ppv);
	}
	else
	{
		return CLASS_E_CLASSNOTAVAILABLE;
	}
}
catch (...)
{
	return winrt::to_hresult();
}
_Use_decl_annotations_ STDAPI DllCanUnloadNow(void)
{
	if (winrt::get_module_lock())
	{
		return S_FALSE;
	}
	else
	{

		return S_OK;
	}
}
