def can_build(env, platform):
    if env.editor_build:
        # The RegEx module is required to parse SemVer in the hostfxr resolver.
        env.module_add_dependencies("dotnet", ["regex"])

    return True


def configure(env):
    import version

    if version.status == "stable":
        raise RuntimeError(
            "The new .NET module is still experimental and should not be included in a stable build of the engine."
        )

    env.add_module_version_string("dotnet")


def is_enabled():
    # The module is disabled by default. Use module_dotnet_enabled=yes to enable it.
    return False
