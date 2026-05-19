#ifndef vtkViennaPSBuiltinModels_h
#define vtkViennaPSBuiltinModels_h

#include "vtkViennaPSModelRegistry.h"
#include "vtkViennaPSUtils.h"
#include "vtkViennaPSLogger.h"
#include "vtkViennaPSMetadata.h"

#include <psDomain.hpp>
#include <materials/psMaterialMap.hpp>
#include <process/psProcess.hpp>

#include <memory>
#include <type_traits>

namespace ViennaPSModels {

using NumericType = ViennaPSMeta::NumericType;
constexpr int D = ViennaPSMeta::D;

// Dispatch helper: calls `fn(typedDomain, dimTag)` with the correct dimension.
// `dimTag` is std::integral_constant<int, Dim> so Dim is available at compile time.
template<typename Fn>
void withDomain(std::shared_ptr<void> psDomainVoid, int dimension, Fn&& fn) {
    if (dimension == 2) {
        auto psDomain = std::static_pointer_cast<viennaps::Domain<NumericType, 2>>(psDomainVoid);
        fn(psDomain, std::integral_constant<int, 2>{});
    } else if (dimension == 3) {
        auto psDomain = std::static_pointer_cast<viennaps::Domain<NumericType, D>>(psDomainVoid);
        fn(psDomain, std::integral_constant<int, D>{});
    } else {
        VPSLOG_ERROR(nullptr, "Invalid dimension: ", dimension);
    }
}

template<int Dim>
void convertToVTK(
    std::shared_ptr<viennaps::Domain<NumericType, Dim>> psDomain,
    vtkDataObject* output,
    const ViennaPSMeta::ParameterMap& params)
{
    if (!output) return;
    auto& registry = vtkViennaPSModelRegistry::getInstance();
    int outputFormatInt = registry.getParameter<int>(params, "outputFormat", 0);
    auto outputFormat = static_cast<ViennaPSMeta::OutputFormatType>(outputFormatInt);
    vtkViennaPSUtils::ConvertDomainToVTK<NumericType, Dim>(psDomain, output, outputFormat);
}

void initializeGeometryModels();
void initializeEtchingModels();
void initializeDepositionModels();
void initializeDistributionModels();

} // namespace ViennaPSModels

#endif
