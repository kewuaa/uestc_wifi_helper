#pragma once
#include <windows.h>
#include <netlistmgr.h>
#include <wil/stl.h>
#include <wil/com.h>
#include <wrl.h>


namespace nlm {

template<typename Callback>
class NetworkConnectivityListener final
: public Microsoft::WRL::RuntimeClass<
    Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
    INetworkListManagerEvents
> {
public:
    NetworkConnectivityListener(const NetworkConnectivityListener&) = delete;
    NetworkConnectivityListener& operator=(const NetworkConnectivityListener&) = delete;

    NetworkConnectivityListener(INetworkListManager* networkListManager, Callback callback):
        m_networkListManager(networkListManager), m_callback(std::move(callback)) {
    }

    IFACEMETHODIMP ConnectivityChanged(NLM_CONNECTIVITY connectivity) noexcept override
    try {
        m_callback(connectivity);
        return S_OK;
    } CATCH_RETURN();

private:
    wil::com_ptr<INetworkListManager> m_networkListManager;
    Callback m_callback;
};

class NLM {
public:
    using unique_connectionpoint_token = wil::unique_com_token<IConnectionPoint, DWORD, decltype(&IConnectionPoint::Unadvise), &IConnectionPoint::Unadvise>;

    enum class Connectivity {
        Connected,
        Disconnected,
        CaptivePortal
    };

    Connectivity check_connectivity() const;
    Connectivity check_connectivity(NLM_CONNECTIVITY connectivity) const;

    template<typename Callback>
    [[nodiscard]] unique_connectionpoint_token register_callback(Callback&& callback) const {
        auto sink = Microsoft::WRL::Make<NetworkConnectivityListener<Callback>>(nlm_.get(), std::forward<Callback>(callback));
        wil::com_ptr<IConnectionPointContainer> container = wil::com_query<IConnectionPointContainer>(nlm_.get());
        wil::com_ptr<IConnectionPoint> connectionPoint;
        THROW_IF_FAILED(container->FindConnectionPoint(__uuidof(INetworkListManagerEvents), connectionPoint.put()));

        unique_connectionpoint_token token{ connectionPoint.get() };
        THROW_IF_FAILED(connectionPoint->Advise(sink.Get(), token.put()));
        return token;
    }
private:
    const wil::com_ptr<INetworkListManager> nlm_{ wil::CoCreateInstance<NetworkListManager, INetworkListManager>() };
};

}
