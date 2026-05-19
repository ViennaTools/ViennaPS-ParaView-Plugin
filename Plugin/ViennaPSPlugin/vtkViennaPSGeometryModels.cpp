#include "vtkViennaPSBuiltinModels.h"

#include <geometries/psMakeFin.hpp>
#include <geometries/psMakeHole.hpp>
#include <geometries/psMakePlane.hpp>
#include <geometries/psMakeStack.hpp>
#include <geometries/psMakeTrench.hpp>

#include <iostream>

namespace {
using NumericType = ViennaPSMeta::NumericType;
constexpr int D = ViennaPSMeta::D;

void registerTrenchModel() {
  using namespace ViennaPSMeta;

  ModelMetadata trenchMeta;
  trenchMeta.className = "MakeTrench";
  trenchMeta.displayName = "Trench Geometry";
  trenchMeta.type = ModelType::GEOMETRY;
  auto materialNames = getAllMaterialNames();
  trenchMeta.parameters = {
      {"trenchWidth",
       "Trench Width",
       "Width of the trench",
       ParameterType::DOUBLE,
       4.0,
       0.5,
       1e6,
       {},
       {},
       ParameterCategory::BASIC,
       "",
       "",
       0.1,
       true},
      {"trenchDepth",
       "Trench Depth",
       "Depth of the trench",
       ParameterType::DOUBLE,
       8.0,
       1.0,
       1e6,
       {},
       {},
       ParameterCategory::BASIC,
       "",
       "",
       0.5,
       true},
      {"taperAngle",
       "Taper Angle",
       "Sidewall taper angle (degrees)",
       ParameterType::DOUBLE,
       0.0,
       -30.0,
       30.0,
       {},
       {},
       ParameterCategory::BASIC,
       "",
       "\u00b0",
       0.5,
       false},
      {"material",
       "Material",
       "Substrate material",
       ParameterType::ENUM,
       getMaterialIndex(viennaps::Material::Si),
       0,
       static_cast<int>(materialNames.size() - 1),
       materialNames,
       {},
       ParameterCategory::BASIC,
       "",
       "",
       1.0,
       true},
      {"maskHeight",
       "Mask Height",
       "Height of the mask above the trench. Set > 0 to create mask.",
       ParameterType::DOUBLE,
       0.0,
       0.0,
       1e6,
       {},
       {},
       ParameterCategory::ADVANCED,
       "",
       "",
       0.1,
       false},
      {"maskTaperAngle",
       "Mask Taper Angle",
       "Taper angle of the mask opening (degrees)",
       ParameterType::DOUBLE,
       0.0,
       -30.0,
       30.0,
       {},
       {},
       ParameterCategory::ADVANCED,
       "",
       "\u00b0",
       0.5,
       false},
      {"halfTrench",
       "Half Trench",
       "Create half trench (symmetry along x-axis)",
       ParameterType::BOOLEAN,
       false,
       false,
       true,
       {},
       {},
       ParameterCategory::ADVANCED,
       "",
       "",
       1.0,
       false},
      {"maskMaterial",
       "Mask Material",
       "Material for the mask",
       ParameterType::ENUM,
       getMaterialIndex(viennaps::Material::Mask),
       0,
       static_cast<int>(materialNames.size() - 1),
       materialNames,
       {},
       ParameterCategory::ADVANCED,
       "",
       "",
       1.0,
       false},
      {"periodicBoundary",
       "Periodic Boundary",
       "Enable periodic boundary conditions",
       ParameterType::BOOLEAN,
       false,
       false,
       true,
       {},
       {},
       ParameterCategory::ADVANCED,
       "",
       "",
       1.0,
       false},
      {"useMultiLayer",
       "Multi-Layer Mode",
       "Use multi-layer configuration instead of single trench",
       ParameterType::BOOLEAN,
       false,
       false,
       true,
       {},
       {},
       ParameterCategory::ADVANCED,
       "",
       "",
       1.0,
       false},
      {"numLayers",
       "Number of Layers",
       "Number of material layers (multi-layer mode)",
       ParameterType::INTEGER,
       2,
       1,
       10,
       {},
       {},
       ParameterCategory::ADVANCED,
       "",
       "",
       1.0,
       false}};

  // Add per-layer parameters (Layer1_ through Layer10_)
  for (int i = 1; i <= 10; i++) {
    std::string prefix = "Layer" + std::to_string(i) + "_";
    std::string label = "Layer " + std::to_string(i) + " ";

    trenchMeta.parameters.push_back({prefix + "Height",
                                     label + "Height",
                                     "Height of layer " + std::to_string(i),
                                     ParameterType::DOUBLE,
                                     5.0,
                                     0.1,
                                     1e6,
                                     {},
                                     {},
                                     ParameterCategory::ADVANCED,
                                     "",
                                     "",
                                     0.5,
                                     false});
    trenchMeta.parameters.push_back(
        {prefix + "Width",
         label + "Width",
         "Cutout width of layer " + std::to_string(i),
         ParameterType::DOUBLE,
         4.0,
         0.1,
         1e6,
         {},
         {},
         ParameterCategory::ADVANCED,
         "",
         "",
         0.5,
         false});
    trenchMeta.parameters.push_back(
        {prefix + "TaperAngle",
         label + "Taper Angle",
         "Taper angle for layer " + std::to_string(i) + " (degrees)",
         ParameterType::DOUBLE,
         0.0,
         -30.0,
         30.0,
         {},
         {},
         ParameterCategory::ADVANCED,
         "",
         "\u00b0",
         0.5,
         false});
    trenchMeta.parameters.push_back({prefix + "Material",
                                     label + "Material",
                                     "Material for layer " + std::to_string(i),
                                     ParameterType::ENUM,
                                     getMaterialIndex(viennaps::Material::Si),
                                     0,
                                     static_cast<int>(materialNames.size() - 1),
                                     materialNames,
                                     {},
                                     ParameterCategory::ADVANCED,
                                     "",
                                     "",
                                     1.0,
                                     false});
    trenchMeta.parameters.push_back(
        {prefix + "IsMask",
         label + "Is Mask",
         "Apply cutout to layer " + std::to_string(i),
         ParameterType::BOOLEAN,
         false,
         false,
         true,
         {},
         {},
         ParameterCategory::ADVANCED,
         "",
         "",
         1.0,
         false});
  }

  auto trenchFactory = [](std::shared_ptr<void> psDomainVoid, int dimension,
                          vtkDataObject *output, const ParameterMap &params) {
    auto &registry = vtkViennaPSModelRegistry::getInstance();

    bool periodicBoundary =
        registry.getParameter<bool>(params, "periodicBoundary", false);
    bool useMultiLayer =
        registry.getParameter<bool>(params, "useMultiLayer", false);
    bool halfTrench = registry.getParameter<bool>(params, "halfTrench", false);

    ViennaPSModels::withDomain(
        psDomainVoid, dimension, [&](auto psDomain, auto dimTag) {
          constexpr int Dim = decltype(dimTag)::value;
          double gridDelta = psDomain->getGrid().getGridDelta();
          double xExtent =
              2.0 * psDomain->getGrid().getMaxGridPoint(0) * gridDelta;
          double yExtent =
              (Dim == 3)
                  ? 2.0 * psDomain->getGrid().getMaxGridPoint(1) * gridDelta
                  : 0.0;

          psDomain->setup(gridDelta, xExtent, yExtent,
                          periodicBoundary
                              ? viennaps::BoundaryType::PERIODIC_BOUNDARY
                              : viennaps::BoundaryType::REFLECTIVE_BOUNDARY);

          if (useMultiLayer) {
            int numLayers = registry.getParameter<int>(params, "numLayers", 2);
            auto materialNames = getAllMaterialNames();

            std::vector<
                typename viennaps::MakeTrench<NumericType, Dim>::MaterialLayer>
                layers;
            for (int i = 1; i <= numLayers; i++) {
              std::string prefix = "Layer" + std::to_string(i) + "_";
              typename viennaps::MakeTrench<NumericType, Dim>::MaterialLayer
                  layer;
              layer.height =
                  registry.getParameter<double>(params, prefix + "Height", 5.0);
              layer.width =
                  registry.getParameter<double>(params, prefix + "Width", 4.0);
              layer.taperAngle = registry.getParameter<double>(
                  params, prefix + "TaperAngle", 0.0);
              int layerMatIndex =
                  registry.getParameter<int>(params, prefix + "Material", 0);
              if (layerMatIndex >= 0 &&
                  layerMatIndex < static_cast<int>(materialNames.size())) {
                layer.material =
                    getMaterialFromString(materialNames[layerMatIndex]);
              }
              layer.isMask =
                  registry.getParameter<bool>(params, prefix + "IsMask", false);
              layers.push_back(layer);
            }

            viennaps::MakeTrench<NumericType, Dim> trench(psDomain, layers,
                                                          halfTrench);
            trench.apply();
          } else {
            double width =
                registry.getParameter<double>(params, "trenchWidth", 4.0);
            double depth =
                registry.getParameter<double>(params, "trenchDepth", 8.0);
            double taperAngle =
                registry.getParameter<double>(params, "taperAngle", 0.0);
            double maskHeight =
                registry.getParameter<double>(params, "maskHeight", 0.0);
            double maskTaperAngle =
                registry.getParameter<double>(params, "maskTaperAngle", 0.0);

            int matIndex = registry.getParameter<int>(params, "material", 0);
            auto materialNames = getAllMaterialNames();
            viennaps::Material material = viennaps::Material::Si;
            if (matIndex >= 0 &&
                matIndex < static_cast<int>(materialNames.size())) {
              material = getMaterialFromString(materialNames[matIndex]);
            }

            int maskMatIndex =
                registry.getParameter<int>(params, "maskMaterial", 0);
            viennaps::Material maskMaterial = viennaps::Material::Mask;
            if (maskMatIndex >= 0 &&
                maskMatIndex < static_cast<int>(materialNames.size())) {
              maskMaterial = getMaterialFromString(materialNames[maskMatIndex]);
            }

            viennaps::MakeTrench<NumericType, Dim> trench(
                psDomain, width, depth, taperAngle, maskHeight, maskTaperAngle,
                halfTrench, material, maskMaterial);
            trench.apply();
          }
          ViennaPSModels::convertToVTK<Dim>(psDomain, output, params);
        });
  };

  vtkViennaPSModelRegistry::getInstance().registerGeometryModel(
      "Trench", trenchMeta, trenchFactory);
}

void registerStackModel() {
  using namespace ViennaPSMeta;

  ModelMetadata stackMeta;
  stackMeta.className = "MakeStack";
  stackMeta.displayName = "Stack Geometry";
  stackMeta.description =
      "Creates alternating SiO2/Si3N4 layers with optional hole/trench";
  stackMeta.type = ModelType::GEOMETRY;
  auto materialNames = getAllMaterialNames();

  stackMeta.parameters = {
      {"numLayers",
       "Number of Layers",
       "Number of alternating layers",
       ParameterType::INTEGER,
       4,
       1,
       20,
       {},
       {},
       ParameterCategory::BASIC,
       "",
       "",
       1.0,
       true},
      {"layerHeight",
       "Layer Height",
       "Height of each layer",
       ParameterType::DOUBLE,
       5.0,
       0.5,
       1e6,
       {},
       {},
       ParameterCategory::BASIC,
       "",
       "",
       0.5,
       true},
      {"substrateHeight",
       "Substrate Height",
       "Height of the substrate",
       ParameterType::DOUBLE,
       10.0,
       1.0,
       1e6,
       {},
       {},
       ParameterCategory::BASIC,
       "",
       "",
       0.5,
       true},

      {"holeRadius",
       "Hole Radius",
       "Radius of the hole (3D) or half-width of trench (2D)",
       ParameterType::DOUBLE,
       0.0,
       0.0,
       1e6,
       {},
       {},
       ParameterCategory::BASIC,
       "",
       "",
       0.5,
       false},
      {"trenchWidth",
       "Trench Width",
       "Width of the trench (overrides holeRadius in 2D)",
       ParameterType::DOUBLE,
       0.0,
       0.0,
       1e6,
       {},
       {},
       ParameterCategory::BASIC,
       "",
       "",
       0.5,
       false},

      {"maskHeight",
       "Mask Height",
       "Height of the top mask layer",
       ParameterType::DOUBLE,
       0.0,
       0.0,
       1e6,
       {},
       {},
       ParameterCategory::MASK,
       "",
       "",
       0.1,
       false},

      {"taperAngle",
       "Taper Angle",
       "Taper angle for hole/trench (degrees)",
       ParameterType::DOUBLE,
       0.0,
       -30.0,
       30.0,
       {},
       {},
       ParameterCategory::ADVANCED,
       "",
       "\u00b0",
       0.5,
       false},
      {"halfStack",
       "Half Stack",
       "Create half stack (symmetry along x-axis)",
       ParameterType::BOOLEAN,
       false,
       false,
       true,
       {},
       {},
       ParameterCategory::ADVANCED,
       "",
       "",
       1.0,
       false},
      {"maskMaterial",
       "Mask Material",
       "Material for the mask",
       ParameterType::ENUM,
       getMaterialIndex(viennaps::Material::Mask),
       0,
       static_cast<int>(materialNames.size() - 1),
       materialNames,
       {},
       ParameterCategory::ADVANCED,
       "",
       "",
       1.0,
       false},
      {"periodicBoundary",
       "Periodic Boundary",
       "Enable periodic boundary conditions",
       ParameterType::BOOLEAN,
       false,
       false,
       true,
       {},
       {},
       ParameterCategory::ADVANCED,
       "",
       "",
       1.0,
       false}};

  auto stackFactory = [](std::shared_ptr<void> psDomainVoid, int dimension,
                         vtkDataObject *output, const ParameterMap &params) {
    auto &registry = vtkViennaPSModelRegistry::getInstance();

    int numLayers = registry.getParameter<int>(params, "numLayers", 4);
    double layerHeight =
        registry.getParameter<double>(params, "layerHeight", 5.0);
    double substrateHeight =
        registry.getParameter<double>(params, "substrateHeight", 10.0);
    double holeRadius =
        registry.getParameter<double>(params, "holeRadius", 0.0);
    double trenchWidth =
        registry.getParameter<double>(params, "trenchWidth", 0.0);
    double maskHeight =
        registry.getParameter<double>(params, "maskHeight", 0.0);
    double taperAngle =
        registry.getParameter<double>(params, "taperAngle", 0.0);
    bool halfStack = registry.getParameter<bool>(params, "halfStack", false);
    bool periodicBoundary =
        registry.getParameter<bool>(params, "periodicBoundary", false);

    int maskMatIndex = registry.getParameter<int>(params, "maskMaterial", 0);
    auto materialNames = getAllMaterialNames();
    viennaps::Material maskMaterial = viennaps::Material::Mask;
    if (maskMatIndex >= 0 &&
        maskMatIndex < static_cast<int>(materialNames.size())) {
      maskMaterial = getMaterialFromString(materialNames[maskMatIndex]);
    }

    ViennaPSModels::withDomain(
        psDomainVoid, dimension, [&](auto psDomain, auto dimTag) {
          constexpr int Dim = decltype(dimTag)::value;
          double gridDelta = psDomain->getGrid().getGridDelta();
          double xExtent =
              2.0 * psDomain->getGrid().getMaxGridPoint(0) * gridDelta;
          double yExtent =
              (Dim == 3)
                  ? 2.0 * psDomain->getGrid().getMaxGridPoint(1) * gridDelta
                  : 0.0;

          psDomain->setup(gridDelta, xExtent, yExtent,
                          periodicBoundary
                              ? viennaps::BoundaryType::PERIODIC_BOUNDARY
                              : viennaps::BoundaryType::REFLECTIVE_BOUNDARY);

          viennaps::MakeStack<NumericType, Dim> stack(
              psDomain, numLayers, layerHeight, substrateHeight, holeRadius,
              trenchWidth, maskHeight, taperAngle, halfStack, maskMaterial);
          stack.apply();
          ViennaPSModels::convertToVTK<Dim>(psDomain, output, params);
        });
  };

  vtkViennaPSModelRegistry::getInstance().registerGeometryModel(
      "Stack", stackMeta, stackFactory);
}

void registerFinModel() {
  using namespace ViennaPSMeta;

  ModelMetadata finMeta;
  finMeta.className = "MakeFin";
  finMeta.displayName = "Fin Geometry";
  finMeta.description = "Creates a fin structure extending in z/y direction";
  finMeta.type = ModelType::GEOMETRY;
  auto materialNames = getAllMaterialNames();
  finMeta.parameters = {
      {"finWidth",
       "Fin Width",
       "Width of the fin",
       ParameterType::DOUBLE,
       10.0,
       1.0,
       1e6,
       {},
       {},
       ParameterCategory::BASIC,
       "",
       "",
       0.5,
       true},
      {"finHeight",
       "Fin Height",
       "Height of the fin",
       ParameterType::DOUBLE,
       20.0,
       1.0,
       1e6,
       {},
       {},
       ParameterCategory::BASIC,
       "",
       "",
       0.5,
       true},
      {"taperAngle",
       "Taper Angle",
       "Sidewall taper angle (degrees)",
       ParameterType::DOUBLE,
       0.0,
       -45.0,
       45.0,
       {},
       {},
       ParameterCategory::BASIC,
       "",
       "\u00b0",
       0.5,
       false},
      {"material",
       "Material",
       "Fin material",
       ParameterType::ENUM,
       getMaterialIndex(viennaps::Material::Si),
       0,
       static_cast<int>(materialNames.size() - 1),
       materialNames,
       {},
       ParameterCategory::BASIC,
       "",
       "",
       1.0,
       true},
      {"maskHeight",
       "Mask Height",
       "Height of the mask above the fin. Set > 0 to create mask.",
       ParameterType::DOUBLE,
       0.0,
       0.0,
       1e6,
       {},
       {},
       ParameterCategory::ADVANCED,
       "",
       "",
       0.1,
       false},
      {"maskTaperAngle",
       "Mask Taper Angle",
       "Taper angle of the mask (degrees)",
       ParameterType::DOUBLE,
       0.0,
       -30.0,
       30.0,
       {},
       {},
       ParameterCategory::ADVANCED,
       "",
       "\u00b0",
       0.5,
       false},
      {"halfFin",
       "Half Fin",
       "Create half fin (symmetry along x-axis)",
       ParameterType::BOOLEAN,
       false,
       false,
       true,
       {},
       {},
       ParameterCategory::ADVANCED,
       "",
       "",
       1.0,
       false},
      {"maskMaterial",
       "Mask Material",
       "Material for the mask",
       ParameterType::ENUM,
       getMaterialIndex(viennaps::Material::Mask),
       0,
       static_cast<int>(materialNames.size() - 1),
       materialNames,
       {},
       ParameterCategory::ADVANCED,
       "",
       "",
       1.0,
       false},
      {"periodicBoundary",
       "Periodic Boundary",
       "Enable periodic boundary conditions",
       ParameterType::BOOLEAN,
       false,
       false,
       true,
       {},
       {},
       ParameterCategory::ADVANCED,
       "",
       "",
       1.0,
       false}};

  auto finFactory = [](std::shared_ptr<void> psDomainVoid, int dimension,
                       vtkDataObject *output, const ParameterMap &params) {
    auto &registry = vtkViennaPSModelRegistry::getInstance();

    double width = registry.getParameter<double>(params, "finWidth", 10.0);
    double height = registry.getParameter<double>(params, "finHeight", 20.0);
    double taperAngle =
        registry.getParameter<double>(params, "taperAngle", 0.0);
    double maskHeight =
        registry.getParameter<double>(params, "maskHeight", 0.0);
    double maskTaperAngle =
        registry.getParameter<double>(params, "maskTaperAngle", 0.0);
    bool halfFin = registry.getParameter<bool>(params, "halfFin", false);
    bool periodicBoundary =
        registry.getParameter<bool>(params, "periodicBoundary", false);

    int matIndex = registry.getParameter<int>(params, "material", 0);
    auto materialNames = getAllMaterialNames();
    viennaps::Material material = viennaps::Material::Si;
    if (matIndex >= 0 && matIndex < static_cast<int>(materialNames.size())) {
      material = getMaterialFromString(materialNames[matIndex]);
    }

    int maskMatIndex = registry.getParameter<int>(params, "maskMaterial", 0);
    viennaps::Material maskMaterial = viennaps::Material::Mask;
    if (maskMatIndex >= 0 &&
        maskMatIndex < static_cast<int>(materialNames.size())) {
      maskMaterial = getMaterialFromString(materialNames[maskMatIndex]);
    }

    ViennaPSModels::withDomain(
        psDomainVoid, dimension, [&](auto psDomain, auto dimTag) {
          constexpr int Dim = decltype(dimTag)::value;
          double gridDelta = psDomain->getGrid().getGridDelta();
          double xExtent =
              2.0 * psDomain->getGrid().getMaxGridPoint(0) * gridDelta;
          double yExtent =
              (Dim == 3)
                  ? 2.0 * psDomain->getGrid().getMaxGridPoint(1) * gridDelta
                  : 0.0;

          psDomain->setup(gridDelta, xExtent, yExtent,
                          periodicBoundary
                              ? viennaps::BoundaryType::PERIODIC_BOUNDARY
                              : viennaps::BoundaryType::REFLECTIVE_BOUNDARY);

          viennaps::MakeFin<NumericType, Dim> fin(
              psDomain, width, height, taperAngle, maskHeight, maskTaperAngle,
              halfFin, material, maskMaterial);
          fin.apply();
          ViennaPSModels::convertToVTK<Dim>(psDomain, output, params);
        });
  };

  vtkViennaPSModelRegistry::getInstance().registerGeometryModel("Fin", finMeta,
                                                                finFactory);
}

void registerHoleModel() {
  using namespace ViennaPSMeta;

  ModelMetadata holeMeta;
  holeMeta.className = "MakeHole";
  holeMeta.displayName = "Hole Geometry";
  holeMeta.description = "Creates a hole (3D) or trench (2D) structure";
  holeMeta.type = ModelType::GEOMETRY;
  auto materialNames = getAllMaterialNames();
  holeMeta.parameters = {
      {"holeRadius",
       "Hole Radius",
       "Radius of the hole",
       ParameterType::DOUBLE,
       5.0,
       0.5,
       1e6,
       {},
       {},
       ParameterCategory::BASIC,
       "",
       "",
       0.5,
       true},
      {"holeDepth",
       "Hole Depth",
       "Depth of the hole",
       ParameterType::DOUBLE,
       10.0,
       1.0,
       1e6,
       {},
       {},
       ParameterCategory::BASIC,
       "",
       "",
       0.5,
       true},
      {"taperAngle",
       "Taper Angle",
       "Sidewall taper angle (degrees)",
       ParameterType::DOUBLE,
       0.0,
       -45.0,
       45.0,
       {},
       {},
       ParameterCategory::BASIC,
       "",
       "\u00b0",
       0.5,
       false},
      {"material",
       "Material",
       "Substrate material",
       ParameterType::ENUM,
       getMaterialIndex(viennaps::Material::Si),
       0,
       static_cast<int>(materialNames.size() - 1),
       materialNames,
       {},
       ParameterCategory::BASIC,
       "",
       "",
       1.0,
       true},
      {"holeShape",
       "Hole Shape",
       "Shape of the hole",
       ParameterType::ENUM,
       0,
       0,
       2,
       {"Full", "Half", "Quarter"},
       {{0, "Full"}, {1, "Half"}, {2, "Quarter"}},
       ParameterCategory::BASIC,
       "",
       "",
       1.0,
       false},
      {"maskHeight",
       "Mask Height",
       "Height of the mask above the hole. Set > 0 to create mask.",
       ParameterType::DOUBLE,
       0.0,
       0.0,
       1e6,
       {},
       {},
       ParameterCategory::ADVANCED,
       "",
       "",
       0.1,
       false},
      {"maskTaperAngle",
       "Mask Taper Angle",
       "Taper angle of the mask opening (degrees)",
       ParameterType::DOUBLE,
       0.0,
       -30.0,
       30.0,
       {},
       {},
       ParameterCategory::ADVANCED,
       "",
       "\u00b0",
       0.5,
       false},
      {"maskMaterial",
       "Mask Material",
       "Material for the mask",
       ParameterType::ENUM,
       getMaterialIndex(viennaps::Material::Mask),
       0,
       static_cast<int>(materialNames.size() - 1),
       materialNames,
       {},
       ParameterCategory::ADVANCED,
       "",
       "",
       1.0,
       false},
      {"periodicBoundary",
       "Periodic Boundary",
       "Enable periodic boundary conditions",
       ParameterType::BOOLEAN,
       false,
       false,
       true,
       {},
       {},
       ParameterCategory::ADVANCED,
       "",
       "",
       1.0,
       false}};

  auto holeFactory = [](std::shared_ptr<void> psDomainVoid, int dimension,
                        vtkDataObject *output, const ParameterMap &params) {
    auto &registry = vtkViennaPSModelRegistry::getInstance();

    double radius = registry.getParameter<double>(params, "holeRadius", 5.0);
    double depth = registry.getParameter<double>(params, "holeDepth", 10.0);
    double taperAngle =
        registry.getParameter<double>(params, "taperAngle", 0.0);
    double maskHeight =
        registry.getParameter<double>(params, "maskHeight", 0.0);
    double maskTaperAngle =
        registry.getParameter<double>(params, "maskTaperAngle", 0.0);
    bool periodicBoundary =
        registry.getParameter<bool>(params, "periodicBoundary", false);

    int matIndex = registry.getParameter<int>(params, "material", 0);
    auto materialNames = getAllMaterialNames();
    viennaps::Material material = viennaps::Material::Si;
    if (matIndex >= 0 && matIndex < static_cast<int>(materialNames.size())) {
      material = getMaterialFromString(materialNames[matIndex]);
    }

    int maskMatIndex = registry.getParameter<int>(params, "maskMaterial", 0);
    viennaps::Material maskMaterial = viennaps::Material::Mask;
    if (maskMatIndex >= 0 &&
        maskMatIndex < static_cast<int>(materialNames.size())) {
      maskMaterial = getMaterialFromString(materialNames[maskMatIndex]);
    }

    int shapeIndex = registry.getParameter<int>(params, "holeShape", 0);
    viennaps::HoleShape shape = static_cast<viennaps::HoleShape>(shapeIndex);

    ViennaPSModels::withDomain(
        psDomainVoid, dimension, [&](auto psDomain, auto dimTag) {
          constexpr int Dim = decltype(dimTag)::value;
          double gridDelta = psDomain->getGrid().getGridDelta();
          double xExtent =
              2.0 * psDomain->getGrid().getMaxGridPoint(0) * gridDelta;
          double yExtent =
              (Dim == 3)
                  ? 2.0 * psDomain->getGrid().getMaxGridPoint(1) * gridDelta
                  : 0.0;

          psDomain->setup(gridDelta, xExtent, yExtent,
                          periodicBoundary
                              ? viennaps::BoundaryType::PERIODIC_BOUNDARY
                              : viennaps::BoundaryType::REFLECTIVE_BOUNDARY);

          viennaps::MakeHole<NumericType, Dim> hole(
              psDomain, radius, depth, taperAngle, maskHeight, maskTaperAngle,
              shape, material, maskMaterial);
          hole.apply();
          ViennaPSModels::convertToVTK<Dim>(psDomain, output, params);
        });
  };

  vtkViennaPSModelRegistry::getInstance().registerGeometryModel(
      "Hole", holeMeta, holeFactory);
}

void registerPlaneModel() {
  using namespace ViennaPSMeta;

  ModelMetadata planeMeta;
  planeMeta.className = "MakePlane";
  planeMeta.displayName = "Planar Substrate";
  planeMeta.description = "Creates a flat substrate surface";
  planeMeta.type = ModelType::GEOMETRY;
  auto materialNames = getAllMaterialNames();
  planeMeta.parameters = {
      {"baseHeight",
       "Base Height",
       "Height of the substrate surface",
       ParameterType::DOUBLE,
       0.0,
       -50.0,
       1e6,
       {},
       {},
       ParameterCategory::BASIC,
       "",
       "",
       0.5,
       true},
      {"material",
       "Material",
       "Material of the substrate",
       ParameterType::ENUM,
       getMaterialIndex(viennaps::Material::Si),
       0,
       static_cast<int>(materialNames.size() - 1),
       materialNames,
       {},
       ParameterCategory::BASIC,
       "",
       "",
       1.0,
       true},
      {"periodicBoundary",
       "Periodic Boundary",
       "Enable periodic boundary conditions",
       ParameterType::BOOLEAN,
       false,
       false,
       true,
       {},
       {},
       ParameterCategory::ADVANCED,
       "",
       "",
       1.0,
       false},
      {"addToExisting",
       "Add to Existing",
       "Add plane to existing geometry instead of creating new",
       ParameterType::BOOLEAN,
       false,
       false,
       true,
       {},
       {},
       ParameterCategory::ADVANCED,
       "",
       "",
       1.0,
       false}};

  auto planeFactory = [](std::shared_ptr<void> psDomainVoid, int dimension,
                         vtkDataObject *output, const ParameterMap &params) {
    auto &registry = vtkViennaPSModelRegistry::getInstance();

    double baseHeight =
        registry.getParameter<double>(params, "baseHeight", 0.0);
    bool periodicBoundary =
        registry.getParameter<bool>(params, "periodicBoundary", false);
    bool addToExisting =
        registry.getParameter<bool>(params, "addToExisting", false);

    int matIndex = registry.getParameter<int>(params, "material", 0);
    auto materialNames = getAllMaterialNames();
    viennaps::Material material = viennaps::Material::Si;

    if (matIndex >= 0 && matIndex < static_cast<int>(materialNames.size())) {
      material = getMaterialFromString(materialNames[matIndex]);
    }

    ViennaPSModels::withDomain(
        psDomainVoid, dimension, [&](auto psDomain, auto dimTag) {
          constexpr int Dim = decltype(dimTag)::value;
          double gridDelta = psDomain->getGrid().getGridDelta();
          double xExtent =
              2.0 * psDomain->getGrid().getMaxGridPoint(0) * gridDelta;
          double yExtent =
              (Dim == 3)
                  ? 2.0 * psDomain->getGrid().getMaxGridPoint(1) * gridDelta
                  : 0.0;

          psDomain->setup(gridDelta, xExtent, yExtent,
                          periodicBoundary
                              ? viennaps::BoundaryType::PERIODIC_BOUNDARY
                              : viennaps::BoundaryType::REFLECTIVE_BOUNDARY);

          viennaps::MakePlane<NumericType, Dim> plane(psDomain, baseHeight,
                                                      material, addToExisting);
          plane.apply();
          ViennaPSModels::convertToVTK<Dim>(psDomain, output, params);
        });
  };

  vtkViennaPSModelRegistry::getInstance().registerGeometryModel(
      "Plane", planeMeta, planeFactory);
}

} // anonymous namespace

void ViennaPSModels::initializeGeometryModels() {
  registerTrenchModel();
  registerHoleModel();
  registerStackModel();
  registerFinModel();
  registerPlaneModel();
  VPSLOG_DEBUG(nullptr, "Registered geometry models");
}
