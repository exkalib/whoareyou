import unreal


ROOT = "/Game/WorldSim"
MAP_PATH = ROOT + "/Maps/L_WorldSimSmoke"
WIDGET_PATH = ROOT + "/UI/WBP_WorldSimDebug"
CONTROLLER_PATH = ROOT + "/Blueprints/BP_WorldSimSmokeController"


def load_native_class(name):
    result = unreal.load_class(None, "/Script/WorldSimDemo." + name)
    if result is None:
        raise RuntimeError("Native class is unavailable: " + name)
    return result


def create_blueprint(asset_path, parent_class, asset_class, factory_class):
    existing = unreal.EditorAssetLibrary.load_asset(asset_path)
    if existing is not None:
        return existing

    package_path, asset_name = asset_path.rsplit("/", 1)
    factory = factory_class()
    factory.set_editor_property("parent_class", parent_class)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, package_path, asset_class, factory
    )
    if asset is None:
        raise RuntimeError("Failed to create asset: " + asset_path)
    return asset


def main():
    widget_parent = load_native_class("WorldSimSmokeWidget")
    controller_parent = load_native_class("WorldSimSmokeController")

    widget = create_blueprint(
        WIDGET_PATH,
        widget_parent,
        unreal.WidgetBlueprint,
        unreal.WidgetBlueprintFactory,
    )
    controller = create_blueprint(
        CONTROLLER_PATH,
        controller_parent,
        unreal.Blueprint,
        unreal.BlueprintFactory,
    )

    controller_cdo = unreal.get_default_object(controller.generated_class())
    controller_cdo.set_editor_property("widget_class", widget.generated_class())
    unreal.KismetEditorUtilities.compile_blueprint(widget)
    unreal.KismetEditorUtilities.compile_blueprint(controller)

    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
    else:
        if not unreal.EditorLevelLibrary.new_level(MAP_PATH):
            raise RuntimeError("Failed to create map: " + MAP_PATH)

    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    if not any(actor.get_class() == controller.generated_class() for actor in actors):
        unreal.EditorLevelLibrary.spawn_actor_from_class(
            controller.generated_class(), unreal.Vector(0.0, 0.0, 0.0)
        )

    unreal.EditorAssetLibrary.save_loaded_asset(widget)
    unreal.EditorAssetLibrary.save_loaded_asset(controller)
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log("E023 assets created successfully: " + MAP_PATH)


main()
