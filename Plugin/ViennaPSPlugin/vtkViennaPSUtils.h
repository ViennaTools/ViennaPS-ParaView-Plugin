#ifndef vtkViennaPSUtils_h
#define vtkViennaPSUtils_h

#include <vtkPolyData.h>
#include <vtkDataObject.h>
#include <vtkSmartPointer.h>
#include <memory>
#include <vtkViennaPSMetadata.h>

#include <psDomain.hpp>

namespace vtkViennaPSUtils {

    template<typename NumericType, int D>
    void ConvertDomainToVTK(
        std::shared_ptr<viennaps::Domain<NumericType, D>> psDomain,
        vtkDataObject* output,
        ViennaPSMeta::OutputFormatType outputFormat = ViennaPSMeta::OutputFormatType::VOLUME);

    template<typename NumericType, int D>
    std::shared_ptr<viennaps::Domain<NumericType, D>> ConvertVTKToDomain(
        vtkDataObject* input,
        double gridDelta,
        double xExtent,
        double yExtent);

    template<typename NumericType, int D>
    std::shared_ptr<viennaps::Domain<NumericType, D>> GetDomainFromInput(
        vtkDataObject* input,
        double gridDelta = 0.0,
        double xExtent = 0.0,
        double yExtent = 0.0);

}

#endif
