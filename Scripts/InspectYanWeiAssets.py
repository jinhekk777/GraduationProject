import unreal


ASSET_PATHS = [
    "/Game/Mesh/yanwei_sun/SM_yanwei_sun_Tenon",
    "/Game/Mesh/yanwei_sun/SM_yanwei_sun_Mortise",
]


def describe_mesh(path):
    mesh = unreal.load_asset(path)
    if not mesh:
        raise RuntimeError("Unable to load {}".format(path))

    values = {"path": mesh.get_path_name()}
    for name in ("get_bounds", "get_bounding_box"):
        try:
            values[name] = str(getattr(mesh, name)())
        except Exception:
            pass
    for name in ("light_map_resolution", "allow_cpu_access", "nanite_settings"):
        try:
            values[name] = str(mesh.get_editor_property(name))
        except Exception:
            pass
    unreal.log("YanWei asset: {}".format(values))


for asset_path in ASSET_PATHS:
    describe_mesh(asset_path)
