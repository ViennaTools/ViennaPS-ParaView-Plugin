#include "vtkViennaPSUtils.h"
#include "vtkViennaPSLogger.h"
#include "vtkViennaPSDomainObject.h"
#include <algorithm>
#include <vcSmartPointer.hpp>

#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkUnstructuredGrid.h>
#include <vtkAppendFilter.h>
#include <vtkInformation.h>

#include <psDomain.hpp>
#include "psSurfacePointValuesToLevelSet.hpp"
#include <lsToSurfaceMesh.hpp>
#include <lsToDiskMesh.hpp>
#include <lsFromSurfaceMesh.hpp>
#include <lsMesh.hpp>
#include <lsVTKWriter.hpp>
#include <lsVTKReader.hpp>

namespace vtkViennaPSUtils {

template<typename NumericType, int D>
void ConvertDomainToVTK(
    std::shared_ptr<viennaps::Domain<NumericType, D>> psDomain,
    vtkDataObject* output,
    ViennaPSMeta::OutputFormatType outputFormat)
{

    switch (outputFormat)
    {
    case ViennaPSMeta::OutputFormatType::VOLUME:
        {
            constexpr double DEFAULT_WRAPPING_EPSILON = 1e-2;
            
            viennals::WriteVisualizationMesh<NumericType, D> writer;
            writer.setWrappingLayerEpsilon(DEFAULT_WRAPPING_EPSILON);
            writer.setWriteToFile(false);
            for (auto &ls : psDomain->getLevelSets()) {
              writer.insertNextLevelSet(ls);
            }
            if (psDomain->getMaterialMap())
            writer.setMaterialMap(psDomain->getMaterialMap()->getMaterialMap());
            writer.setMetaData(psDomain->getMetaData());
            writer.apply();
            vtkUnstructuredGrid* outputUGrid = vtkUnstructuredGrid::SafeDownCast(output);
            if (outputUGrid) {
                vtkUnstructuredGrid *result = writer.getVolumeMesh();
                outputUGrid->ShallowCopy(result);
            }
        }
        break;
    case ViennaPSMeta::OutputFormatType::SURFACE:
        {
            if (psDomain->getLevelSets().empty()) {
                VPSLOG_ERROR(nullptr, "Cannot extract surface mesh: domain has no level sets");
                break;
            }
            auto mesh = viennacore::SmartPointer<viennals::Mesh<NumericType>>::New();
            viennals::ToDiskMesh<NumericType, D> meshConverter;
            meshConverter.setMesh(mesh);
            if (psDomain->getMaterialMap())
              meshConverter.setMaterialMap(psDomain->getMaterialMap()->getMaterialMap());
            for (const auto ls : psDomain->getLevelSets()) {
              meshConverter.insertNextLevelSet(ls);
            }
            meshConverter.apply();
            viennaps::SurfacePointValuesToLevelSet<NumericType, D>(psDomain->getLevelSets().back(), mesh,
                                                         {"MaterialIds"}).apply();

            viennals::ToSurfaceMesh<NumericType, D>(psDomain->getLevelSets().back(), mesh).apply();
            viennals::VTKWriter<NumericType> writer(mesh);
            writer.setMetaData(psDomain->getMetaData());

            vtkUnstructuredGrid* outputUGrid = vtkUnstructuredGrid::SafeDownCast(output);
            vtkPolyData* outputPolyData = vtkPolyData::SafeDownCast(output);
            if (outputUGrid) {
                writer.setFileFormat(viennals::FileFormatEnum::VTU);
                writer.apply();
                vtkUnstructuredGrid *result = writer.getUnstructuredGrid();
                if (result) outputUGrid->ShallowCopy(result);
            } else if (outputPolyData) {
                writer.apply();
                vtkPolyData *result = writer.getPolyData();
                if (result) outputPolyData->ShallowCopy(result);
            }
        }
        break;
    case ViennaPSMeta::OutputFormatType::HULL:
        {
            constexpr double DEFAULT_WRAPPING_EPSILON = 1e-2;

            viennals::WriteVisualizationMesh<NumericType, D> writer;
            writer.setWrappingLayerEpsilon(DEFAULT_WRAPPING_EPSILON);
            writer.setExtractHullMesh(true);
            writer.setExtractVolumeMesh(false);
            writer.setWriteToFile(false);
            for (auto &ls : psDomain->getLevelSets()) {
              writer.insertNextLevelSet(ls);
            }
            if (psDomain->getMaterialMap())
              writer.setMaterialMap(psDomain->getMaterialMap()->getMaterialMap());
            writer.setMetaData(psDomain->getMetaData());
            writer.apply();

            // Hull mesh is always a PolyData from WriteVisualizationMesh
            vtkPolyData *hullResult = writer.getHullMesh();
            vtkUnstructuredGrid* outputUGrid = vtkUnstructuredGrid::SafeDownCast(output);
            vtkPolyData* outputPolyData = vtkPolyData::SafeDownCast(output);
            if (outputUGrid && hullResult) {
                vtkNew<vtkAppendFilter> appendFilter;
                appendFilter->AddInputData(hullResult);
                appendFilter->Update();
                outputUGrid->ShallowCopy(appendFilter->GetOutput());
            } else if (outputPolyData && hullResult) {
                outputPolyData->ShallowCopy(hullResult);
            }
        }
        break;
    default:
        break;
    }
    VPSLOG_DEBUG(nullptr, "Domain converted to VTK.");
}

template<typename NumericType, int D>
std::shared_ptr<viennaps::Domain<NumericType, D>> ConvertVTKToDomain(
    vtkDataObject* input,
    double gridDelta,
    double xExtent,
    double yExtent)
{
    VPSLOG_DEBUG(nullptr, "ConvertVTKToDomain called");

        if (gridDelta <= 0 || xExtent <= 0 || yExtent <= 0) {
            VPSLOG_INFO(nullptr, "Invalid domain parameters, extracting from mesh bounds");

            double bounds[6] = {0, 0, 0, 0, 0, 0};
            vtkPolyData* polyInput = vtkPolyData::SafeDownCast(input);
            vtkUnstructuredGrid* ugridInput = vtkUnstructuredGrid::SafeDownCast(input);

            if (polyInput) {
                polyInput->GetBounds(bounds);
            } else if (ugridInput) {
                ugridInput->GetBounds(bounds);
            }

            double meshXExtent = bounds[1] - bounds[0];
            double meshYExtent = bounds[3] - bounds[2];

            if (xExtent <= 0) xExtent = meshXExtent;
            if (yExtent <= 0) yExtent = meshYExtent;

            if (gridDelta <= 0) {
                // Use 1% of the smallest extent as grid delta
                gridDelta = std::min(xExtent, yExtent) * 0.01;
                if (gridDelta <= 0) gridDelta = 0.25;
            }

            VPSLOG_INFO(nullptr, "Extracted from mesh - Grid: ", gridDelta,
                       ", XExtent: ", xExtent, ", YExtent: ", yExtent);
    } else {
        VPSLOG_INFO(nullptr, "Using provided domain parameters - Grid: ",
                   gridDelta, ", XExtent: ", xExtent, ", YExtent: ", yExtent);
    }

    auto psDomain = std::make_shared<viennaps::Domain<NumericType, D>>(
        gridDelta, xExtent, yExtent
    );
    
    auto mesh = viennacore::SmartPointer<viennals::Mesh<NumericType>>::New();

    vtkPolyData* polyInput = vtkPolyData::SafeDownCast(input);
    vtkUnstructuredGrid* ugridInput = vtkUnstructuredGrid::SafeDownCast(input);
    
    viennals::VTKReader<NumericType> reader(mesh);
    if (polyInput) {
        VPSLOG_DEBUG(nullptr, "Reading from vtkPolyData");
        reader.setPolyData(polyInput);
    } else if (ugridInput) {
        VPSLOG_DEBUG(nullptr, "Reading from vtkUnstructuredGrid");
        reader.setUnstructuredGrid(ugridInput);
    } else {
        VPSLOG_ERROR(nullptr, "Input is neither PolyData nor UnstructuredGrid!");
        return psDomain;
    }
    reader.apply();

    auto levelSet = viennacore::SmartPointer<viennals::Domain<NumericType, D>>::New(
        psDomain->getGrid()
    );

    viennals::FromSurfaceMesh<NumericType, D> mesher;
    mesher.setMesh(mesh);
    mesher.setLevelSet(levelSet);
    mesher.apply();

    psDomain->insertNextLevelSetAsMaterial(
        levelSet,
        viennaps::Material::Si
    );

    VPSLOG_DEBUG(nullptr, "VTK converted to Domain.");
    return psDomain;
}

template<typename NumericType, int D>
std::shared_ptr<viennaps::Domain<NumericType, D>> GetDomainFromInput(
    vtkDataObject* input,
    double gridDelta,
    double xExtent,
    double yExtent)
{
    if (!input) {
        VPSLOG_ERROR(nullptr, "GetDomainFromInput: Input is null");
        return nullptr;
    }

    vtkViennaPSDomainObject* domainObj = vtkViennaPSDomainObject::SafeDownCast(input);
    if (domainObj) {
        VPSLOG_INFO(nullptr, "GetDomainFromInput: Input is vtkViennaPSDomainObject, creating deep copy");

        auto sourceDomain = domainObj->GetDomain<NumericType, D>();
        if (!sourceDomain) {
            VPSLOG_ERROR(nullptr, "GetDomainFromInput: Failed to get domain from vtkViennaPSDomainObject");
            return nullptr;
        }

        auto copiedDomain = std::make_shared<viennaps::Domain<NumericType, D>>();

        // SmartPointer is derived from shared_ptr, so we can construct from shared_ptr
        viennacore::SmartPointer<viennaps::Domain<NumericType, D>> smartPtr(sourceDomain);
        copiedDomain->deepCopy(smartPtr);

        VPSLOG_DEBUG(nullptr, "GetDomainFromInput: Deep copy created successfully");
        return copiedDomain;
    }

    VPSLOG_INFO(nullptr, "GetDomainFromInput: Input is mesh, converting to domain");
    return ConvertVTKToDomain<NumericType, D>(input, gridDelta, xExtent, yExtent);
}

template void ConvertDomainToVTK<double, 3>(
    std::shared_ptr<viennaps::Domain<double, 3>>, vtkDataObject*, ViennaPSMeta::OutputFormatType);
template void ConvertDomainToVTK<double, 2>(
    std::shared_ptr<viennaps::Domain<double, 2>>, vtkDataObject*, ViennaPSMeta::OutputFormatType);

template std::shared_ptr<viennaps::Domain<double, 3>> ConvertVTKToDomain<double, 3>(
    vtkDataObject*, double, double, double);
template std::shared_ptr<viennaps::Domain<double, 2>> ConvertVTKToDomain<double, 2>(
    vtkDataObject*, double, double, double);

template std::shared_ptr<viennaps::Domain<double, 3>> GetDomainFromInput<double, 3>(
    vtkDataObject*, double, double, double);
template std::shared_ptr<viennaps::Domain<double, 2>> GetDomainFromInput<double, 2>(
    vtkDataObject*, double, double, double);

} // namespace vtkViennaPSUtils