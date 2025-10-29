#include <functional>

#include "nlm.hpp"


namespace nlm {

NLM::Connectivity NLM::check_connectivity() const {
    NLM_CONNECTIVITY connectivity = NLM_CONNECTIVITY_DISCONNECTED;
    THROW_IF_FAILED(
        nlm_->GetConnectivity(&connectivity));
    return check_connectivity(connectivity);
}

NLM::Connectivity NLM::check_connectivity(NLM_CONNECTIVITY connectivity) const {
    // check internet connectivity
    if (WI_IsAnyFlagSet(
        connectivity,
        NLM_CONNECTIVITY_IPV4_INTERNET | NLM_CONNECTIVITY_IPV6_INTERNET
    )) {
        return Connectivity::Connected;
    }
    else if (WI_IsAnyFlagSet(
        connectivity,
        NLM_CONNECTIVITY_IPV4_LOCALNETWORK | NLM_CONNECTIVITY_IPV6_LOCALNETWORK
    )) {
        // we are local connected, check if we're behind a captive portal before attempting to connect to the Internet.
        //
        // note: being behind a captive portal means connectivity is local and there is at least one interface(network)
        // behind a captive portal.

        bool localConnectedBehindCaptivePortal = false;
        wil::com_ptr<IEnumNetworks> enumConnectedNetworks;
        THROW_IF_FAILED(
            nlm_->GetNetworks(
                NLM_ENUM_NETWORK_CONNECTED,
                enumConnectedNetworks.put()
            )
        );

        // Enumeration returns S_FALSE when there are no more items.
        wil::com_ptr<INetwork> networkConnection;
        while (THROW_IF_FAILED(enumConnectedNetworks->Next(1, networkConnection.put(), nullptr)) == S_OK) {
            wil::com_ptr<IPropertyBag> networkProperties = networkConnection.query<IPropertyBag>();

            // these might fail if there's no value
            wil::unique_variant variantInternetConnectivityV4;
            networkProperties->Read(NA_InternetConnectivityV4, variantInternetConnectivityV4.addressof(), nullptr);
            wil::unique_variant variantInternetConnectivityV6;
            networkProperties->Read(NA_InternetConnectivityV6, variantInternetConnectivityV6.addressof(), nullptr);

            // read the VT_UI4 from the VARIANT and cast it to a NLM_INTERNET_CONNECTIVITY
            // If there is no value, then assume no special treatment.
            NLM_INTERNET_CONNECTIVITY v4Connectivity = static_cast<NLM_INTERNET_CONNECTIVITY>(variantInternetConnectivityV4.vt == VT_UI4 ? variantInternetConnectivityV4.ulVal : 0);
            NLM_INTERNET_CONNECTIVITY v6Connectivity = static_cast<NLM_INTERNET_CONNECTIVITY>(variantInternetConnectivityV6.vt == VT_UI4 ? variantInternetConnectivityV6.ulVal : 0);

            if (
                WI_IsFlagSet(v4Connectivity, NLM_INTERNET_CONNECTIVITY_WEBHIJACK)
                || WI_IsFlagSet(v6Connectivity, NLM_INTERNET_CONNECTIVITY_WEBHIJACK)
            ) {
                // at least one connected interface is behind a captive portal
                // we should assume that the device is behind it
                localConnectedBehindCaptivePortal = true;
            }
        }
        return localConnectedBehindCaptivePortal ? Connectivity::CaptivePortal : Connectivity::Connected;
    }
    else {
        return Connectivity::Disconnected;
    }
}

}
