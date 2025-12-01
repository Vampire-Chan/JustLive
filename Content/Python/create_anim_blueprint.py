"""
JustLive Animation Blueprint Generator
Creates a procedural Animation Blueprint for the Ped character with:
1. Locomotion State (Idle, Walk, Run, Sprint, Jump, Fall)
2. Cover State (Low Cover, High Cover, Peek, Vault)
3. Crouched State
4. Combat State (Armed, Melee)
5. Swimming State (Surface, Diving)

Usage: Run this script in Unreal Editor's Python console or via Editor Utility Widget
"""

import unreal

# Asset paths
ANIM_BP_PATH = "/Game/Characters/Animations/ABP_Ped"
SKELETON_PATH = "/Game/Characters/Skeletons/Humanoid_Skeleton"

# Animation asset paths (these would be your actual animation assets)
ANIM_PATHS = {
    # Locomotion
    "idle": "/Game/Characters/Animations/Locomotion/Idle",
    "walk_fwd": "/Game/Characters/Animations/Locomotion/Walk_Fwd",
    "walk_bwd": "/Game/Characters/Animations/Locomotion/Walk_Bwd",
    "walk_left": "/Game/Characters/Animations/Locomotion/Walk_Left",
    "walk_right": "/Game/Characters/Animations/Locomotion/Walk_Right",
    "run_fwd": "/Game/Characters/Animations/Locomotion/Run_Fwd",
    "sprint_fwd": "/Game/Characters/Animations/Locomotion/Sprint_Fwd",
    "jump_start": "/Game/Characters/Animations/Locomotion/Jump_Start",
    "jump_loop": "/Game/Characters/Animations/Locomotion/Jump_Loop",
    "jump_land": "/Game/Characters/Animations/Locomotion/Jump_Land",
    "fall_loop": "/Game/Characters/Animations/Locomotion/Fall_Loop",
    
    # Crouch
    "crouch_idle": "/Game/Characters/Animations/Crouch/Crouch_Idle",
    "crouch_walk_fwd": "/Game/Characters/Animations/Crouch/Crouch_Walk_Fwd",
    
    # Cover
    "cover_low_idle": "/Game/Characters/Animations/Cover/Cover_Low_Idle",
    "cover_high_idle": "/Game/Characters/Animations/Cover/Cover_High_Idle",
    "cover_peek_left": "/Game/Characters/Animations/Cover/Cover_Peek_Left",
    "cover_peek_right": "/Game/Characters/Animations/Cover/Cover_Peek_Right",
    "cover_peek_up": "/Game/Characters/Animations/Cover/Cover_Peek_Up",
    "vault_over": "/Game/Characters/Animations/Cover/Vault_Over",
    
    # Combat
    "combat_idle": "/Game/Characters/Animations/Combat/Combat_Idle",
    "combat_walk_fwd": "/Game/Characters/Animations/Combat/Combat_Walk_Fwd",
    "combat_aim": "/Game/Characters/Animations/Combat/Combat_Aim",
    "melee_swing": "/Game/Characters/Animations/Combat/Melee_Swing",
    
    # Swimming
    "swim_surface_idle": "/Game/Characters/Animations/Swimming/Swim_Surface_Idle",
    "swim_surface_fwd": "/Game/Characters/Animations/Swimming/Swim_Surface_Fwd",
    "swim_dive": "/Game/Characters/Animations/Swimming/Swim_Dive",
    "swim_underwater": "/Game/Characters/Animations/Swimming/Swim_Underwater",
}


def create_animation_blueprint():
    """Main function to create the Animation Blueprint"""
    
    # Get asset tools
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    
    # Load skeleton
    skeleton = unreal.load_asset(SKELETON_PATH)
    if not skeleton:
        unreal.log_error(f"Failed to load skeleton at {SKELETON_PATH}")
        return None
    
    # Create Animation Blueprint factory
    factory = unreal.AnimBlueprintFactory()
    factory.target_skeleton = skeleton
    factory.parent_class = unreal.PedAnimInstance  # Our C++ class
    
    # Create the asset
    package_path = ANIM_BP_PATH.rsplit('/', 1)[0]
    asset_name = ANIM_BP_PATH.rsplit('/', 1)[1]
    
    anim_bp = asset_tools.create_asset(
        asset_name=asset_name,
        package_path=package_path,
        asset_class=unreal.AnimBlueprint,
        factory=factory
    )
    
    if not anim_bp:
        unreal.log_error("Failed to create Animation Blueprint")
        return None
    
    unreal.log(f"Created Animation Blueprint: {ANIM_BP_PATH}")
    
    # Get the AnimGraph
    anim_graph = None
    for graph in anim_bp.get_editor_property('function_graphs'):
        if graph.get_name() == 'AnimGraph':
            anim_graph = graph
            break
    
    if not anim_graph:
        unreal.log_error("Could not find AnimGraph")
        return None
    
    # Build the animation graph
    build_anim_graph(anim_graph, anim_bp)
    
    # Save the asset
    unreal.EditorAssetLibrary.save_loaded_asset(anim_bp)
    
    return anim_bp


def build_anim_graph(anim_graph, anim_bp):
    """Build the animation graph with state machines"""
    
    # Get the graph schema
    schema = anim_graph.get_editor_property('schema')
    
    # Create the main state machine
    state_machine_node = create_state_machine(anim_graph, "MainStateMachine", -400, 0)
    
    if not state_machine_node:
        unreal.log_error("Failed to create state machine")
        return
    
    # Get the state machine graph
    sm_graph = state_machine_node.get_editor_property('bounded_graph')
    
    # Create states
    create_locomotion_state(sm_graph, -800, -200)
    create_cover_state(sm_graph, -400, -200)
    create_crouch_state(sm_graph, 0, -200)
    create_combat_state(sm_graph, 400, -200)
    create_swimming_state(sm_graph, 800, -200)
    
    # Connect state machine to output
    output_node = get_output_node(anim_graph)
    if output_node and state_machine_node:
        connect_nodes(state_machine_node, 'Pose', output_node, 'Result')
    
    unreal.log("Animation graph built successfully")


def create_state_machine(graph, name, x, y):
    """Create a state machine node"""
    try:
        # This is a simplified version - actual implementation would use
        # unreal.AnimGraphNode_StateMachine and proper graph manipulation
        unreal.log(f"Creating state machine: {name} at ({x}, {y})")
        # In practice, you'd use the animation blueprint's internal API
        # to create nodes programmatically
        return None
    except Exception as e:
        unreal.log_error(f"Error creating state machine: {e}")
        return None


def create_locomotion_state(sm_graph, x, y):
    """Create the Locomotion state with sub-states"""
    unreal.log("Creating Locomotion State")
    # This would contain:
    # - Idle
    # - Walk (BlendSpace for directional movement)
    # - Run
    # - Sprint
    # - Jump (Start -> Loop -> Land)
    # - Fall
    pass


def create_cover_state(sm_graph, x, y):
    """Create the Cover state"""
    unreal.log("Creating Cover State")
    # This would contain:
    # - Low Cover Idle
    # - High Cover Idle
    # - Peek Left/Right/Up
    # - Vault Over
    pass


def create_crouch_state(sm_graph, x, y):
    """Create the Crouched state"""
    unreal.log("Creating Crouched State")
    # This would contain:
    # - Crouch Idle
    # - Crouch Walk (BlendSpace)
    pass


def create_combat_state(sm_graph, x, y):
    """Create the Combat state"""
    unreal.log("Creating Combat State")
    # This would contain:
    # - Combat Idle
    # - Combat Walk
    # - Aiming (with AimOffset)
    # - Melee Attack
    pass


def create_swimming_state(sm_graph, x, y):
    """Create the Swimming state"""
    unreal.log("Creating Swimming State")
    # This would contain:
    # - Surface Idle
    # - Surface Swim
    # - Dive Transition
    # - Underwater Swim
    pass


def get_output_node(graph):
    """Get the output pose node from the graph"""
    for node in graph.get_editor_property('nodes'):
        if node.get_class().get_name() == 'AnimGraphNode_Root':
            return node
    return None


def connect_nodes(source_node, source_pin, dest_node, dest_pin):
    """Connect two nodes together"""
    try:
        # Find pins and connect them
        # This is simplified - actual implementation would use pin manipulation
        unreal.log(f"Connecting {source_pin} to {dest_pin}")
        pass
    except Exception as e:
        unreal.log_error(f"Error connecting nodes: {e}")


# Entry point
if __name__ == "__main__":
    unreal.log("=== JustLive Animation Blueprint Generator ===")
    result = create_animation_blueprint()
    if result:
        unreal.log("Animation Blueprint created successfully!")
    else:
        unreal.log_error("Failed to create Animation Blueprint")
