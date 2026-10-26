#pragma once

#include <cmath>
#include <algorithm>
#include "../../Globals.cpp"
#include "../../Addresses.cpp"

// =======================
// EngineInterfaces
// =======================

// Access to the interfaces the engine gives to the game code, used by the console debug tools
namespace EngineInterface
{
    enum class Slot
    {
        // Client and server
        GetObjectRotation,
        GetObjectPos,
        SetObjectPos,
        RegisterConsoleProgram,
        GetConsoleVariable,
        GetConsoleVariableFloat,
        SetConsoleVariableFloat,
        GetConsoleVariableString,
        GetSourceWorldOffset,

        // Client
        GetModelLT,
        PhysicsSim,
        GetRenderer,
        GetCustomRender,
        SendToServer,

        // Server
        GetClientObject,
        GetObjectName,
        GetClassName,

        Count
    };

    // Byte offsets in the virtual table: FEAR, FEARMP, FEARXP, FEARXP2
    constexpr uint16_t kSlotTable[static_cast<size_t>(Slot::Count)][4] =
    {
        /* GetObjectRotation          */ {  0x18,  0x18,  0x18,  0x18 },
        /* GetObjectPos               */ {  0x20,  0x20,  0x20,  0x20 },
        /* SetObjectPos               */ {  0x24,  0x24,  0x24,  0x24 },
        /* RegisterConsoleProgram     */ {  0x9C,  0x9C,  0x9C,  0x9C },
        /* GetConsoleVariable         */ { 0x10C, 0x10C, 0x10C, 0x110 },
        /* GetConsoleVariableFloat    */ { 0x114, 0x114, 0x114, 0x118 },
        /* SetConsoleVariableFloat    */ { 0x118, 0x118, 0x118, 0x11C },
        /* GetConsoleVariableString   */ { 0x11C, 0x11C, 0x11C, 0x120 },
        /* GetSourceWorldOffset       */ { 0x134, 0x134, 0x134, 0x138 },
        /* GetModelLT                 */ {  0x44,  0x44,  0x44,  0x44 },
        /* PhysicsSim                 */ {  0x58,  0x58,  0x58,  0x58 },
        /* GetRenderer                */ { 0x16C, 0x16C, 0x16C, 0x170 },
        /* GetCustomRender            */ { 0x170, 0x170, 0x170, 0x174 },
        /* SendToServer               */ { 0x1B4, 0x1B4, 0x1B4, 0x1B8 },
        /* GetClientObject            */ { 0x188, 0x188, 0x188, 0x18C },
        /* GetObjectName              */ { 0x1BC, 0x1BC, 0x1BC, 0x1C0 },
        /* GetClassName               */ { 0x1D4, 0x1D4, 0x1D4, 0x1D8 },
    };

    // The interfaces also have functions stored as members, called without 'this'
    constexpr DWORD Client_GetClientObject = 0x84;
    constexpr DWORD Server_GetNextClient = 0x1C;
    constexpr DWORD Server_GetClass = 0xC4;
    constexpr DWORD Server_GetObjectClass = 0xC8;

    // Common interface
    constexpr DWORD Common_CreateMessage = 0x38;

    // Messages, reference counted
    constexpr DWORD Message_AddRef = 0x04;
    constexpr DWORD Message_Release = 0x08;
    constexpr DWORD Message_Read = 0x1C;
    constexpr DWORD Message_Size = 0x20;
    constexpr DWORD Message_WriteBits = 0x24;
    constexpr DWORD Message_WriteData = 0x2C;
    constexpr DWORD Message_WriteString = 0x38;
    constexpr DWORD Message_ReadData = 0x44;

    // Database
    constexpr DWORD Database_GetCategory = 0x1C;
    constexpr DWORD Database_GetCategoryParent = 0x28;
    constexpr DWORD Database_GetCategoryName = 0x30;
    constexpr DWORD Database_GetRecord = 0x38;
    constexpr DWORD Database_GetRecordParent = 0x44;
    constexpr DWORD Database_GetRecordName = 0x4C;

    // Model
    constexpr DWORD Model_GetNode = 0x2C;
    constexpr DWORD Model_GetNodeName = 0x30;

    // Physics
    constexpr DWORD Physics_IsWorldObject = 0x04;
    constexpr DWORD Physics_GetObjectDims = 0x20;

    // Physics simulation
    constexpr DWORD PhysicsSim_CreateSphereShape = 0x0C;
    constexpr DWORD PhysicsSim_CreateBoxShape = 0x10;
    constexpr DWORD PhysicsSim_CreateCapsuleShape = 0x14;
    constexpr DWORD PhysicsSim_ReleaseShape = 0x2C;
    constexpr DWORD PhysicsSim_TraverseShapeHierarchy = 0x38;
    constexpr DWORD PhysicsSim_CreateRigidBody = 0x3C;
    constexpr DWORD PhysicsSim_ReleaseRigidBody = 0x44;
    constexpr DWORD PhysicsSim_GetRigidBodyShape = 0x48;
    constexpr DWORD PhysicsSim_GetRigidBodyTransform = 0x5C;
    constexpr DWORD PhysicsSim_GetRigidBodyCollisionInfo = 0x74;
    constexpr DWORD PhysicsSim_GetRigidBodyFriction = 0x8C;
    constexpr DWORD PhysicsSim_GetRigidBodyCOR = 0x94;
    constexpr DWORD PhysicsSim_SetRigidBodyVelocity = 0x98;
    constexpr DWORD PhysicsSim_GetRigidBodyVelocity = 0x9C;
    constexpr DWORD PhysicsSim_SetRigidBodyAngularVelocity = 0xA0;
    constexpr DWORD PhysicsSim_GetRigidBodyAngularVelocity = 0xA4;
    constexpr DWORD PhysicsSim_GetUniqueSystemGroup = 0x150;

    // Renderer
    constexpr DWORD Renderer_WorldToScreen = 0x34;
    constexpr DWORD Renderer_MakeCubicEnvMap = 0x58;

    // Custom render
    constexpr DWORD CustomRender_SetVisBoundingSphere = 0x0C;

    // Same layout as the engine vector and quaternion
    struct Vector3
    {
        float x = 0.0f, y = 0.0f, z = 0.0f;

        Vector3 operator+(const Vector3& other) const { return { x + other.x, y + other.y, z + other.z }; }
        Vector3 operator-(const Vector3& other) const { return { x - other.x, y - other.y, z - other.z }; }
        Vector3 operator*(float scale) const { return { x * scale, y * scale, z * scale }; }
        float Dot(const Vector3& other) const { return x * other.x + y * other.y + z * other.z; }
        float Length() const { return sqrtf(Dot(*this)); }
    };

    struct Rotation
    {
        float x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f;

        Rotation operator*(const Rotation& other) const
        {
            return 
            {
                w * other.x + x * other.w + y * other.z - z * other.y,
                w * other.y - x * other.z + y * other.w + z * other.x,
                w * other.z + x * other.y - y * other.x + z * other.w,
                w * other.w - x * other.x - y * other.y - z * other.z
            };
        }

        Vector3 Rotate(const Vector3& v) const
        {
            Vector3 q = { x, y, z };
            Vector3 t = { 2.0f * (q.y * v.z - q.z * v.y), 2.0f * (q.z * v.x - q.x * v.z), 2.0f * (q.x * v.y - q.y * v.x) };
            Vector3 c = { q.y * t.z - q.z * t.y, q.z * t.x - q.x * t.z, q.x * t.y - q.y * t.x };
            return v + t * w + c;
        }

        Vector3 Right() const { return Rotate({ 1.0f, 0.0f, 0.0f }); }
        Vector3 Up() const { return Rotate({ 0.0f, 1.0f, 0.0f }); }
        Vector3 Forward() const { return Rotate({ 0.0f, 0.0f, 1.0f }); }

        void ToEuler(float& pitch, float& yaw, float& roll) const
        {
            float m12 = 2.0f * (y * z - x * w);
            pitch = asinf(std::clamp(-m12, -1.0f, 1.0f));
            yaw = atan2f(2.0f * (x * z + y * w), 1.0f - 2.0f * (x * x + y * y));
            roll = atan2f(2.0f * (x * y + z * w), 1.0f - 2.0f * (x * x + z * z));
        }
    };

    constexpr float kRadiansToDegrees = 180.0f / 3.14159265f;

    inline DWORD GetClient() { return GetAddress(Addr::Console_ClientInterface); }
    inline DWORD GetServer() { return GetAddress(Addr::Console_ServerInterface); }
    inline DWORD GetClientCommon() { return GetAddress(Addr::Console_ClientCommon); }

    inline DWORD GetSlotOffset(Slot slot)
    {
        return kSlotTable[static_cast<size_t>(slot)][g_State.CurrentFEARGame];
    }

    inline DWORD GetVirtual(DWORD object, DWORD offset)
    {
        return *reinterpret_cast<DWORD*>(*reinterpret_cast<DWORD*>(object) + offset);
    }

    template <typename Ret, typename... Args>
    inline Ret Call(DWORD object, Slot slot, Args... args)
    {
        return reinterpret_cast<Ret(__thiscall*)(DWORD, Args...)>(GetVirtual(object, GetSlotOffset(slot)))(object, args...);
    }

    template <typename Ret, typename... Args>
    inline Ret CallOffset(DWORD object, DWORD offset, Args... args)
    {
        return reinterpret_cast<Ret(__thiscall*)(DWORD, Args...)>(GetVirtual(object, offset))(object, args...);
    }

    template <typename Ret, typename... Args>
    inline Ret CallMember(DWORD object, DWORD offset, Args... args)
    {
        return reinterpret_cast<Ret(__cdecl*)(Args...)>(*reinterpret_cast<DWORD*>(object + offset))(args...);
    }

    template <typename Ret, typename... Args>
    inline Ret CallServerMember(DWORD offset, Args... args)
    {
        return CallMember<Ret>(GetServer(), offset, args...);
    }

    template <typename Fn>
    inline bool SafeCall(Fn&& fn)
    {
        __try
        {
            fn();
            return true;
        }
        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            return false;
        }
    }

    // A game is running when the server console exists
    inline bool IsServerRunning()
    {
        DWORD serverConsole = GetAddress(Addr::Console_ServerManager);
        return serverConsole && *reinterpret_cast<DWORD*>(serverConsole) != 0;
    }

    // Console variables of the client or the server interface
    inline DWORD FindVariable(DWORD object, const char* name)
    {
        return object ? Call<DWORD>(object, Slot::GetConsoleVariable, name) : 0;
    }

    inline float GetVariableFloat(DWORD object, const char* name, float defaultValue = 0.0f)
    {
        DWORD var = FindVariable(object, name);
        return var ? Call<float>(object, Slot::GetConsoleVariableFloat, var) : defaultValue;
    }

    inline std::string GetVariableString(DWORD object, const char* name, const char* defaultValue = "")
    {
        DWORD var = FindVariable(object, name);
        const char* value = var ? Call<const char*>(object, Slot::GetConsoleVariableString, var) : nullptr;
        return value ? value : defaultValue;
    }

    // Made like the game makes its own variables, so it shows in the browser
    inline void CreateVariableFloat(DWORD object, const char* name, float defaultValue)
    {
        if (object && !FindVariable(object, name))
        {
            Call<int>(object, Slot::SetConsoleVariableFloat, name, defaultValue);
        }
    }

    // Name of a server object, empty when it has none
    inline std::string GetObjectName(DWORD hObject)
    {
        char name[256] = {};
        if (hObject)
        {
            Call<int>(GetServer(), Slot::GetObjectName, hObject, name, (uint32_t)sizeof(name));
        }

        return name;
    }

    inline std::string GetClassName(DWORD hClass)
    {
        char name[128] = {};
        if (hClass)
        {
            Call<int>(GetServer(), Slot::GetClassName, hClass, name, (uint32_t)sizeof(name));
        }

        return name;
    }

    // Object of the first player on the server
    inline DWORD GetPlayerObject()
    {
        DWORD hClient = CallServerMember<DWORD>(Server_GetNextClient, (DWORD)0);
        return hClient ? Call<DWORD>(GetServer(), Slot::GetClientObject, hClient) : 0;
    }

    // Builds one message for the local server and sends it
    class ServerMessage
    {
    public:
        explicit ServerMessage(uint8_t messageId)
        {
            DWORD common = GetClientCommon();
            if (common && CallOffset<int>(common, Common_CreateMessage, &m_message) == 0 && m_message)
            {
                CallOffset<void>(m_message, Message_AddRef);
                WriteUint8(messageId);
            }
            else
            {
                m_message = 0;
            }
        }

        ~ServerMessage()
        {
            if (m_message)
            {
                CallOffset<void>(m_message, Message_Release);
            }
        }

        ServerMessage(const ServerMessage&) = delete;
        ServerMessage& operator=(const ServerMessage&) = delete;

        bool IsValid() const { return m_message != 0; }

        void WriteUint8(uint8_t value) { WriteBits(value, 8); }
        void WriteUint32(uint32_t value) { WriteBits(value, 32); }

        void WriteBits(uint32_t value, uint32_t bits)
        {
            if (m_message)
            {
                CallOffset<void>(m_message, Message_WriteBits, value, bits);
            }
        }

        void WriteFloats(const float* values, uint32_t count)
        {
            if (m_message)
            {
                CallOffset<void>(m_message, Message_WriteData, (const void*)values, count * 32);
            }
        }

        void WriteString(const char* text)
        {
            if (m_message)
            {
                CallOffset<void>(m_message, Message_WriteString, text);
            }
        }

        bool Send()
        {
            if (!m_message)
                return false;

            DWORD readMessage = CallOffset<DWORD>(m_message, Message_Read);
            if (!readMessage)
                return false;

            CallOffset<void>(readMessage, Message_AddRef);
            int result = Call<int>(GetClient(), Slot::SendToServer, readMessage, (uint32_t)1);
            CallOffset<void>(readMessage, Message_Release);
            return result == 0;
        }

    private:
        DWORD m_message = 0;
    };
}
