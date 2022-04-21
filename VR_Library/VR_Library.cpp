#include "VR_Library.h"
// VR_Library.cpp: Definiert den Einstiegspunkt für die Anwendung.
//

#include "VR_Library.h"
#include <cassert>



vr_error vr_library::init() {

    vr::EVRInitError eError = vr::VRInitError_None;
    vr_error error;

    m_pHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);
    if (eError != vr::VRInitError_None)
    {

        m_pHMD = NULL;
        
        //std::array<char, 1024> buf;
        //sprintf_s(buf.data(), buf.size(), "Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
        error.success = false;
    }

    if (!vr::VRCompositor())
    {
        printf("Compositor initialization failed. See log file for details\n");
        error.success = false;
    }

    //setup vr matrizes
    m_mat4ProjectionLeft = m_pHMD->GetProjectionMatrix(vr::Eye_Left, 0.1f, 800000.0f);
    m_mat4ProjectionRight = m_pHMD->GetProjectionMatrix(vr::Eye_Right, 0.1f, 800000.0f);

    //atm these matrizes need to be inverted from outside
    m_mat4eyePosLeft = m_pHMD->GetEyeToHeadTransform(vr::Eye_Left);
    m_mat4eyePosRight = m_pHMD->GetEyeToHeadTransform(vr::Eye_Right);

    //setup controll er 
    vr::EVRInputError input_error = vr::VRInputError_None;
    input_error = vr::VRInput()->SetActionManifestPath((std::string("C:/User/Schuerstedt/vr_library/VR_Library/vr_library.json")).c_str());
    input_error = vr::VRInput()->GetActionHandle("/actions/demo/in/SelectButton", &m_action_selectButton);

    input_error = vr::VRInput()->GetActionHandle("/actions/demo/in/AnalogInput", &m_action_AnalogInput);

    input_error = vr::VRInput()->GetActionHandle("/actions/demo/in/GripButton", &m_action_GripButton);

    input_error = vr::VRInput()->GetActionHandle("/actions/demo/in/TouchpadButton", &m_action_TouchpadButton);

    input_error = vr::VRInput()->GetActionHandle("/actions/demo/in/MenuButton", &m_action_MenuButton);
    input_error = vr::VRInput()->GetActionSetHandle("/actions/demo", &m_actionsetDemo);
    input_error = vr::VRInput()->GetActionHandle("/actions/demo/in/Hand_Left", &rlHand[Left].m_actionPose);
    input_error = vr::VRInput()->GetActionHandle("/actions/demo/in/Hand_Right", &rlHand[Right].m_actionPose);

    if (input_error != vr::VRInputError_None) {
        printf("Input Error\n");
      
        error.success = false;
    }

    init_overlay();

    return error;
}

void vr_library::activateOverlayLaser()
{
    vr::VROverlay()->SetOverlayFlag(m_ulOverlayHandle, vr::VROverlayFlags_MakeOverlaysInteractiveIfVisible, false);
}

vr_error vr_library::init_overlay()
{
    bool bSuccess = true;
    if (vr::VROverlay())
    {
        std::string sKey("IFC_Overlay");
        vr::VROverlayError overlayError = vr::VROverlay()->CreateOverlay(sKey.c_str(), std::string("IFCExplorer").c_str(), &m_ulOverlayHandle);
        bSuccess = bSuccess && overlayError == vr::VROverlayError_None;
    }

    if (bSuccess)
    {
        vr::EVROverlayError error;
        vr::VROverlay()->SetOverlayWidthInMeters(m_ulOverlayHandle, 0.5f);
        error = vr::VROverlay()->SetOverlayInputMethod(m_ulOverlayHandle, vr::VROverlayInputMethod_Mouse);
        vr::HmdVector2_t vecWindowSize = { m_overlayWidth, m_overlayHeight };
        vr::VROverlay()->SetOverlayMouseScale(m_ulOverlayHandle, &vecWindowSize);
        vr::VROverlay()->SetOverlayFlag(m_ulOverlayHandle, vr::VROverlayFlags_SendVRSmoothScrollEvents, true);
        vr::VROverlay()->SetOverlayFlag(m_ulOverlayHandle, vr::VROverlayFlags_ShowTouchPadScrollWheel, true);
    }

    //set overlay position according to hmd position
    vr::HmdMatrix34_t transform = {
                1.0f, 0.0f, 0.0f, 0.15f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, -1.3f
    };
    vr::VROverlay()->SetOverlayTransformTrackedDeviceRelative(m_ulOverlayHandle, vr::k_unTrackedDeviceIndex_Hmd, &transform);

    

    return vr_error(true);
}

vr_error vr_library::update() {

    update_hmd_matrix_pose();

    handle_input();

    return vr_error(true);

}

void vr_library::submit_frame(const uint32_t left_eye_texture, const uint32_t right_eye_texture) const
{
    assert(vr::VRCompositor()); // did you call init?
    //submit frame to openvr compositor, to draw on hmd
    const vr::Texture_t leftEyeTexture = { (void*)(left_eye_texture), vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
    vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
    const vr::Texture_t rightEyeTexture = { (void*)(uintptr_t)right_eye_texture, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
    vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);
}

void vr_library::submit_frame_overlay(uint32_t overlay_texture) const
{
    vr::Texture_t infoOverlayTexture = { (void*)(uintptr_t)overlay_texture, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };

    vr::VROverlay()->SetOverlayTexture(m_ulOverlayHandle, &infoOverlayTexture);

}

void vr_library::toggle_overlay(bool show) const
{
    if (show) {
        vr::VROverlay()->ShowOverlay(m_ulOverlayHandle);
        vr::VROverlay()->SetOverlayFlag(m_ulOverlayHandle, vr::VROverlayFlags_MakeOverlaysInteractiveIfVisible, true);
    }
    else {
        vr::VROverlay()->HideOverlay(m_ulOverlayHandle);
        vr::VROverlay()->SetOverlayFlag(m_ulOverlayHandle, vr::VROverlayFlags_MakeOverlaysInteractiveIfVisible, false);
    }

}

vr_library::framebuffer_info vr_library::get_framebuffer_info() const
{
    assert(m_pHMD); // did you call init?
    uint32_t render_width, render_height;

    m_pHMD->GetRecommendedRenderTargetSize(&render_width, &render_height);

    return framebuffer_info(render_width, render_height);

}

vr_error vr_library::handle_input()
{

    if (!vr::VRInput())
    {
        return vr_error(false);
    }

    //load controller
        //=> fill controller struct with data

    //generate controller button press events

    update_button_states();

    //generate right controller activated event?
        //=> so controller can be drawn now by host system

    vr::VRActiveActionSet_t actionSet = { 0 };
    actionSet.ulActionSet = m_actionsetDemo;
    vr::VRInput()->UpdateActionState(&actionSet, sizeof(actionSet), 1);


    update_controller_state();

    update_overlay_events();

    return vr_error(true);
}


void vr_library::update_overlay_events() {

    vr::VREvent_t vrEvent;
    while (vr::VROverlay()->PollNextOverlayEvent(m_ulOverlayHandle, &vrEvent, sizeof(vrEvent))) {
        //std::cout << vrEvent.eventType << std::endl;
        switch (vrEvent.eventType)
        {

        case vr::VREvent_ScrollSmooth:
        {
            m_laserMouseWheel = vrEvent.data.scroll.ydelta;
        }
        break;


        case vr::VREvent_MouseButtonDown:
        {
            m_laserMouseTriggered = true;
        }
        break;
        case vr::VREvent_MouseButtonUp:
        {
            m_laserMouseTriggered = false;
        }
        break;
        case vr::VREvent_MouseMove:
        {
            m_laserPosX = vrEvent.data.mouse.x; 
            m_laserPosY = vrEvent.data.mouse.y;
        }
        break;

        case vr::VREvent_ButtonPress:
        {
            //close overlay with menu button 

            //prevent the overlay from closing immediately
            /*
            auto end = std::chrono::system_clock::now();
            auto elapsed = end - start;
            timeHelper += elapsed.count();
            if (timeHelper > 2000.0f) {
                vr::VROverlay()->SetOverlayFlag(m_ulOverlayHandle, vr::VROverlayFlags_MakeOverlaysInteractiveIfVisible, false);
                vr::VROverlay()->HideOverlay(m_ulOverlayHandle);
                timeHelper = 0.0f;
            }
            */
        }
        break;
        }
    }


}

void vr_library::update_button_states() {

    //trigger button pressed
    vr::InputDigitalActionData_t actionData_selectButton;
    vr::VRInput()->GetDigitalActionData(m_action_selectButton, &actionData_selectButton, sizeof(actionData_selectButton), vr::k_ulInvalidInputValueHandle);

    rlHand[Right].b_selectButton_pressed = actionData_selectButton.bActive && actionData_selectButton.bState;


    //menu button pressed
    vr::InputDigitalActionData_t actionData_MenuButton;
    vr::VRInput()->GetDigitalActionData(m_action_MenuButton, &actionData_MenuButton, sizeof(actionData_MenuButton), vr::k_ulInvalidInputValueHandle);

    rlHand[Right].b_menuButton_pressed = actionData_MenuButton.bActive && actionData_MenuButton.bState;// && actionDataMenu.bChanged;
   

    //touchpad pressed
    vr::InputDigitalActionData_t actionData_touchpadButton;
    vr::VRInput()->GetDigitalActionData(m_action_TouchpadButton, &actionData_touchpadButton, sizeof(actionData_touchpadButton), vr::k_ulInvalidInputValueHandle);

    rlHand[Right].b_touchpadButton_pressed = actionData_touchpadButton.bActive && actionData_touchpadButton.bState;
    

    //touchpad position
    vr::InputAnalogActionData_t actionDataAnalog;
    vr::VRInput()->GetAnalogActionData(m_action_AnalogInput, &actionDataAnalog, sizeof(actionDataAnalog), vr::k_ulInvalidInputValueHandle);
    rlHand[Right].m_trackpadPositionX = actionDataAnalog.x;
    rlHand[Right].m_trackpadPositionY = actionDataAnalog.y;

 

    //haptic right
    vr::InputDigitalActionData_t actionData_GripButton;
    vr::VRInput()->GetDigitalActionData(m_action_GripButton, &actionData_GripButton, sizeof(actionData_GripButton), vr::k_ulInvalidInputValueHandle);
    rlHand[Right].b_gripButton_pressed = actionData_GripButton.bActive&& actionData_GripButton.bState;
       
    //generate button event
    std::unique_ptr<vr_event> button_press(new controller_press(rlHand[Right].b_selectButton_pressed, rlHand[Right].b_menuButton_pressed,
        rlHand[Right].b_touchpadButton_pressed, rlHand[Right].b_gripButton_pressed, rlHand[Right].m_trackpadPositionX,
        rlHand[Right].m_trackpadPositionY));
    button_press->type = vr_event_type::CONTROLLER_PRESS;

    //move unique ptr, as it cannot be copied
    vr_events.push_back(std::move(button_press));

}

void vr_library::update_controller_state() {

   
    for (EHand eHand = Left; eHand <= Right; ((int&)eHand)++)
    {
        vr::InputPoseActionData_t poseData;
        vr::EVRInputError error;
        error = vr::VRInput()->GetPoseActionDataForNextFrame(rlHand[eHand].m_actionPose,
            vr::TrackingUniverseStanding, &poseData, sizeof(poseData), vr::k_ulInvalidInputValueHandle);


        if (error != vr::VRInputError_None
            || !poseData.bActive || !poseData.pose.bPoseIsValid)
        {
            rlHand[eHand].m_bShowController = false;
        }
        else
        {
            rlHand[eHand].m_rmat4Pose = poseData.pose.mDeviceToAbsoluteTracking;
            rlHand[eHand].m_bShowController = true;


            vr::InputOriginInfo_t originInfo;
            if (vr::VRInput()->GetOriginTrackedDeviceInfo(poseData.activeOrigin,
                &originInfo, sizeof(originInfo)) == vr::VRInputError_None
                && originInfo.trackedDeviceIndex != vr::k_unTrackedDeviceIndexInvalid)
            {
                //fixed render model
                std::string sRenderModelName = "vr_controller_vive_1_5";
                	
                if (sRenderModelName != rlHand[eHand].m_sRenderModelName)
                {
                    find_or_load_render_model(sRenderModelName.c_str(), eHand);
                    rlHand[eHand].m_sRenderModelName = sRenderModelName;
                }

            }

            std::unique_ptr<vr_event> cont(new controller_activated(&rlHand[eHand]));
            cont->type = vr_event_type::CONTROLLER_ACTIVATED;

            //move unique ptr, as it cannot be copied
            vr_events.push_back(std::move(cont));
        }
    }
}


void vr_library::find_or_load_render_model(const char* pchRenderModelName, EHand ehand) {
    
    //todo: use / safe model when its already loaded
   
    vr::RenderModel_t* pModel;
    vr::EVRRenderModelError error;
    while (1)
    {
        error = vr::VRRenderModels()->LoadRenderModel_Async(pchRenderModelName, &pModel);
        if (error != vr::VRRenderModelError_Loading)
            break;
        _sleep(1);
    }
    
    if (error != vr::VRRenderModelError_None)
    {
        //todo: error handling
        //std::cout << ("Unable to load render model %s - %s\n", pchRenderModelName, vr::VRRenderModels()->GetRenderModelErrorNameFromEnum(error)) << std::endl;
        printf("VRRenderModelError\n");
       // return NULL; // move on to the next tracked device
    }
  
    vr::RenderModel_TextureMap_t* pTexture;
  
    while (1)
    {
        error = vr::VRRenderModels()->LoadTexture_Async(pModel->diffuseTextureId, &pTexture);
        if (error != vr::VRRenderModelError_Loading)
            break;
  
        _sleep(1);
    }
  
    if (error != vr::VRRenderModelError_None)
    {
        //std::cout << ("Unable to load render texture id:%d for render model %s\n", pModel->diffuseTextureId, pchRenderModelName) << std::endl;
        vr::VRRenderModels()->FreeRenderModel(pModel);
        printf("VRRenderModelError\n");
        //return NULL; // move on to the next tracked device
    }
  
    //safe model and texture
    rlHand[ehand].pModel = pModel;
    rlHand[ehand].pTexture = pTexture;
  
     //vr::VRRenderModels()->FreeRenderModel(pModel);
     //vr::VRRenderModels()->FreeTexture(pTexture);
   
}


vr::HmdMatrix34_t vr_library::getProjectionEyeMatrix(vr::Hmd_Eye nEye)
{
    //Todo

    return vr::HmdMatrix34_t();
}


void vr_library::update_hmd_matrix_pose() {

  assert(vr::VRCompositor()); // did you call init?

  vr::VRCompositor()->WaitGetPoses(m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);

  m_rmat4DevicePose[vr::k_unTrackedDeviceIndex_Hmd] = m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking;

  if (m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
  {
    m_mat4HMDPose = m_rmat4DevicePose[vr::k_unTrackedDeviceIndex_Hmd];

    //invert
    //m_mat4HMDPose = glm::inverse(m_mat4HMDPose);
  }

  //create vr_event and push in vector

  std::unique_ptr<vr_event> hmd_pos (new hmd_position(m_mat4HMDPose));
  hmd_pos->type = vr_event_type::HMD_POSITION;


  //move unique ptr, as it cannot be copied
  vr_events.push_back(std::move(hmd_pos));

}
