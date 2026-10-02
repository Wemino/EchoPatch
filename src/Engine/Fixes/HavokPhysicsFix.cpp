#pragma once

#include "../../Globals.cpp"
#include "../../Addresses.cpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <emmintrin.h>

void(__thiscall* PhysicsSimUpdate)(uintptr_t, float, float, unsigned int) = nullptr;
int(__thiscall* PhysicsSimTerm)(uintptr_t) = nullptr;
int(__thiscall* hkWorldStepDeltaTime)(uintptr_t, float) = nullptr;
uint8_t(__thiscall* RigidBodyKeyframe)(uintptr_t, const float*, float) = nullptr;
void* RigidBodySoftKeyframe = nullptr;
int(__thiscall* RigidBodyTerm)(uintptr_t) = nullptr;
void(__thiscall* RigidBodySetKeyframed)(uintptr_t, bool) = nullptr;
int(__thiscall* RigidBodyApplyForce)(uintptr_t, const float*, const float*) = nullptr;
int(__thiscall* RigidBodyApplyTorque)(uintptr_t, const float*) = nullptr;
void(__thiscall* RigidBodyMarkTransformDirty)(uintptr_t) = nullptr;
void(__thiscall* ObjectMarkAttachmentsDirty)(uintptr_t) = nullptr;

// =========================
// Constants
// =========================

// hkWorld
static constexpr uintptr_t OFF_WORLD_ACTIVE_ISLANDS = 0x08;
static constexpr uintptr_t OFF_WORLD_INACTIVE_ISLANDS = 0x14;
static constexpr uintptr_t OFF_WORLD_SOLVER_STEPS = 0x17C;
static constexpr uintptr_t OFF_WORLD_SOLVER_INV_STEPS = 0x180;
static constexpr uintptr_t OFF_WORLD_QUERY_INV_STEPS = 0x1AC;

// hkSimulationIsland
static constexpr uintptr_t OFF_ISLAND_ENTITIES = 0x3C;

// hkRigidBody
static constexpr uintptr_t OFF_ENTITY_WORLD = 0x08;
static constexpr uintptr_t OFF_ENTITY_PROPERTIES = 0x30;
static constexpr uintptr_t OFF_ENTITY_MOTION = 0x3C;

// hkMotion
static constexpr uintptr_t OFF_MOTION_CENTER_OF_MASS_LOCAL = 0x20;
static constexpr uintptr_t OFF_MOTION_ROTATION = 0x30;
static constexpr uintptr_t OFF_MOTION_LINEAR_VELOCITY = 0x40;
static constexpr uintptr_t OFF_MOTION_ANGULAR_VELOCITY = 0x50;
static constexpr uintptr_t OFF_MOTION_CENTER_OF_MASS = 0x70;
static constexpr uintptr_t OFF_MOTION_TRANSFORM = 0x80;
static constexpr size_t VT_MOTION_GET_TYPE = 0x18 / 4;

static constexpr uintptr_t OFF_SIM_WORLD = 0x1C;
static constexpr uintptr_t OFF_RIGID_BODY_OWNER = 0x08;
static constexpr uintptr_t OFF_RIGID_BODY_ENTITY = 0x10;

static constexpr size_t VT_OWNER_ON_PHYSICS_UPDATE = 0x38 / 4;

static constexpr int MOTION_DYNAMIC = 1;
static constexpr int MOTION_STABILIZED_BOX_INERTIA = 5;
static constexpr int MOTION_KEYFRAMED = 6;

static constexpr float GAME_TO_HAVOK_SCALE = 0.01f;
static constexpr int MAX_TICKS_PER_FRAME = 8;
static constexpr float SNAP_DISTANCE = 4.0f;

static constexpr int MAX_SUBSTEPS = 4;
static constexpr float SUBSTEP_TRAVEL = 0.05f;
static constexpr float LIMB_RADIUS = 0.25f;
static constexpr int SUBSTEP_BODY_BUDGET = 256;
static constexpr int MAX_SUBSTEPPED_TICKS_PER_FRAME = 2;

static constexpr float MIN_TICK_TIME = 0.004f;
static constexpr double TIME_SCALE_WINDOW = 0.15;
static constexpr int TIME_SCALE_SAMPLES = 256;
static constexpr double MAX_TIME_SCALE_SAMPLE = 0.1;
static constexpr float TIME_SCALE_SNAP = 0.03f;

static constexpr double TICK_EARLINESS = 0.0015;

// =========================
// Static State
// =========================

struct HkVector4
{
    float x, y, z, w;
};

struct HkArray
{
    uintptr_t* data;
    int size;
    int capacityAndFlags;
};

struct BodyPose
{
    HkVector4 rotation;
    HkVector4 centerOfMass;
    HkVector4 transform[4];
};

struct TimeScaleSample
{
    float scaledTime;
    float realTime;
};

struct PhysicsWorld
{
    uintptr_t sim = 0;
    uintptr_t world = 0;
    double accumulator = 0.0;
    float alpha = 0.0f;
    float frameTime = 0.0f;
    float tickTime = TARGET_FRAME_TIME;
    int64_t lastCounter = 0;
    TimeScaleSample timeScaleSamples[TIME_SCALE_SAMPLES] = {};
    int firstTimeScaleSample = 0;
    int timeScaleSampleCount = 0;
    double scaledTimeSum = 0.0;
    double realTimeSum = 0.0;
    uint32_t tick = 0;
    uint32_t frame = 0;
    int ticksThisFrame = 0;
    int tickIndex = 0;
    int shownBodies = 0;
    bool isUpdating = false;
    bool isStepping = false;
    bool lastUpdateTicked = false;
};

struct SimulatedBody
{
    uintptr_t world = 0;
    uintptr_t motion = 0;
    BodyPose previous{};
    BodyPose current{};
    BodyPose shown{};
    BodyPose restore{};
    uint32_t previousTick = 0;
    uint32_t currentTick = 0;
    bool isShown = false;
};

struct KeyframeRequest
{
    uintptr_t world = 0;
    uintptr_t rigidBody = 0;
    uintptr_t motion = 0;
    uint32_t frame = 0;
    float transform[7] = {};
    HkVector4 position{};
    HkVector4 rotation{};
    float invDeltaTime = 0.0f;
    float weight = 0.0f;
    float maxLinearVelocity = 0.0f;
    float maxAngularVelocity = 0.0f;
    bool isSoft = false;
};

template <typename T>
class EntityMap
{
public:
    T* Find(uintptr_t key)
    {
        if (m_count == 0 || key == 0) return nullptr;

        for (uint32_t i = Slot(key);; i = (i + 1) & m_mask)
        {
            if (m_buckets[i].key == key) return &GetEntry(m_buckets[i].index).value;
            if (m_buckets[i].key == 0) return nullptr;
        }
    }

    T& operator[](uintptr_t key)
    {
        if (T* value = Find(key)) return *value;

        if ((m_count + 1) * 2 > m_buckets.size())
        {
            Rehash(std::max<size_t>(m_buckets.size() * 2, 256));
        }

        uint32_t index;

        if (!m_freeEntries.empty())
        {
            index = m_freeEntries.back();
            m_freeEntries.pop_back();
        }
        else
        {
            index = m_usedEntries++;
            if (index == m_chunks.size() * CHUNK_SIZE) m_chunks.push_back(std::make_unique<Entry[]>(CHUNK_SIZE));
        }

        Entry& entry = GetEntry(index);
        entry.key = key;
        entry.value = T{};

        uint32_t i = Slot(key);
        while (m_buckets[i].key != 0) i = (i + 1) & m_mask;
        m_buckets[i] = { key, index };
        m_count++;

        return entry.value;
    }

    void Erase(uintptr_t key)
    {
        if (m_count == 0 || key == 0) return;

        uint32_t i = Slot(key);

        while (m_buckets[i].key != key)
        {
            if (m_buckets[i].key == 0) return;
            i = (i + 1) & m_mask;
        }

        GetEntry(m_buckets[i].index).key = 0;
        m_freeEntries.push_back(m_buckets[i].index);
        m_count--;

        // Moves the following entries back into the hole, so lookups never stop early
        for (uint32_t j = (i + 1) & m_mask; m_buckets[j].key != 0; j = (j + 1) & m_mask)
        {
            uint32_t home = Slot(m_buckets[j].key);

            if (((j - home) & m_mask) >= ((j - i) & m_mask))
            {
                m_buckets[i] = m_buckets[j];
                i = j;
            }
        }

        m_buckets[i].key = 0;
    }

    template <typename Pred>
    void EraseIf(Pred pred)
    {
        for (uint32_t index = 0; index < m_usedEntries && m_count != 0; index++)
        {
            Entry& entry = GetEntry(index);
            if (entry.key != 0 && pred(entry.value)) Erase(entry.key);
        }
    }

    template <typename Func>
    void ForEach(Func func)
    {
        for (uint32_t index = 0; index < m_usedEntries; index++)
        {
            Entry& entry = GetEntry(index);
            if (entry.key != 0) func(entry.value);
        }
    }

    size_t Size() const
    {
        return m_count;
    }

    void Clear()
    {
        std::fill(m_buckets.begin(), m_buckets.end(), Bucket{});

        for (uint32_t index = 0; index < m_usedEntries; index++)
        {
            GetEntry(index).key = 0;
        }

        m_freeEntries.clear();
        m_usedEntries = 0;
        m_count = 0;
    }

private:
    static constexpr uint32_t CHUNK_SIZE = 64;

    struct Entry
    {
        uintptr_t key = 0;
        T value{};
    };

    struct Bucket
    {
        uintptr_t key = 0;
        uint32_t index = 0;
    };

    Entry& GetEntry(uint32_t index)
    {
        return m_chunks[index / CHUNK_SIZE][index % CHUNK_SIZE];
    }

    uint32_t Slot(uintptr_t key) const
    {
        return (static_cast<uint32_t>(key >> 4) * 0x9E3779B1) >> m_shift;
    }

    void Rehash(size_t size)
    {
        m_buckets.assign(size, Bucket{});
        m_mask = static_cast<uint32_t>(size - 1);
        m_shift = 32;
        for (size_t bits = size; bits > 1; bits >>= 1) m_shift--;

        for (uint32_t index = 0; index < m_usedEntries; index++)
        {
            Entry& entry = GetEntry(index);
            if (entry.key == 0) continue;

            uint32_t i = Slot(entry.key);
            while (m_buckets[i].key != 0) i = (i + 1) & m_mask;
            m_buckets[i] = { entry.key, index };
        }
    }

    std::vector<Bucket> m_buckets;
    std::vector<std::unique_ptr<Entry[]>> m_chunks;
    std::vector<uint32_t> m_freeEntries;
    uint32_t m_usedEntries = 0;
    uint32_t m_count = 0;
    uint32_t m_mask = 0;
    uint32_t m_shift = 32;
};

static PhysicsWorld s_physicsWorlds[4];
static double s_counterFrequency = 0.0;
static EntityMap<SimulatedBody> s_simulatedBodies;
static EntityMap<KeyframeRequest> s_keyframeRequests;
static std::vector<uintptr_t> s_keyframedEntities;
static std::vector<uintptr_t> s_movedOwners;

// =========================
// Havok Helpers
// =========================

template <typename Func>
static void ForEachIslandEntity(uintptr_t world, uintptr_t islandsOffset, Func&& func)
{
    const HkArray* islands = reinterpret_cast<const HkArray*>(world + islandsOffset);

    for (int i = 0; i < islands->size; i++)
    {
        const HkArray* entities = reinterpret_cast<const HkArray*>(islands->data[i] + OFF_ISLAND_ENTITIES);

        for (int j = 0; j < entities->size; j++)
        {
            func(entities->data[j]);
        }
    }
}

// Stops at the first entity func returns false for
template <typename Func>
static bool ForEachIslandEntityWhile(uintptr_t world, uintptr_t islandsOffset, Func&& func)
{
    const HkArray* islands = reinterpret_cast<const HkArray*>(world + islandsOffset);

    for (int i = 0; i < islands->size; i++)
    {
        const HkArray* entities = reinterpret_cast<const HkArray*>(islands->data[i] + OFF_ISLAND_ENTITIES);

        for (int j = 0; j < entities->size; j++)
        {
            if (!func(entities->data[j])) return false;
        }
    }

    return true;
}

template <typename Func>
static void ForEachWorldEntity(uintptr_t world, Func&& func)
{
    ForEachIslandEntity(world, OFF_WORLD_ACTIVE_ISLANDS, func);
    ForEachIslandEntity(world, OFF_WORLD_INACTIVE_ISLANDS, func);
}

static uintptr_t GetEntityWorld(uintptr_t entity)
{
    return *reinterpret_cast<uintptr_t*>(entity + OFF_ENTITY_WORLD);
}

static uintptr_t GetMotion(uintptr_t entity)
{
    return *reinterpret_cast<uintptr_t*>(entity + OFF_ENTITY_MOTION);
}

static uintptr_t GetEntity(uintptr_t rigidBody)
{
    return *reinterpret_cast<uintptr_t*>(rigidBody + OFF_RIGID_BODY_ENTITY);
}

static int GetMotionType(uintptr_t motion)
{
    using GetType = int(__thiscall*)(uintptr_t);
    return (*reinterpret_cast<GetType**>(motion))[VT_MOTION_GET_TYPE](motion);
}

static bool IsDynamicMotion(int type)
{
    return type >= MOTION_DYNAMIC && type <= MOTION_STABILIZED_BOX_INERTIA;
}

static uintptr_t GetRigidBody(uintptr_t entity)
{
    const HkArray* properties = reinterpret_cast<const HkArray*>(entity + OFF_ENTITY_PROPERTIES);

    // Key and value pairs
    for (int i = 0; i < properties->size; i++)
    {
        if (properties->data[i * 2] == 0) return properties->data[i * 2 + 1];
    }

    return 0;
}

// =========================
// SIMD Helpers
// =========================

static __m128 LoadVector(uintptr_t address)
{
    return _mm_loadu_ps(reinterpret_cast<const float*>(address));
}

static __m128 LoadVector(const HkVector4& v)
{
    return _mm_loadu_ps(&v.x);
}

static void StoreVector(HkVector4& v, const __m128& value)
{
    _mm_storeu_ps(&v.x, value);
}

template <int Lane>
static __m128 Splat(const __m128& v)
{
    return _mm_shuffle_ps(v, v, _MM_SHUFFLE(Lane, Lane, Lane, Lane));
}

static __m128 MaskXYZ()
{
    return _mm_castsi128_ps(_mm_set_epi32(0, -1, -1, -1));
}

// x, y and z of a, w of b
static __m128 SelectXYZ(const __m128& a, const __m128& b)
{
    const __m128 mask = MaskXYZ();
    return _mm_or_ps(_mm_and_ps(mask, a), _mm_andnot_ps(mask, b));
}

// ((x + y) + z) + w
static float SumInOrder(const __m128& v)
{
    __m128 sum = _mm_add_ss(v, Splat<1>(v));
    sum = _mm_add_ss(sum, _mm_movehl_ps(v, v));
    return _mm_cvtss_f32(_mm_add_ss(sum, Splat<3>(v)));
}

static void ReadPose(uintptr_t motion, BodyPose& pose)
{
    const __m128i* rotation = reinterpret_cast<const __m128i*>(motion + OFF_MOTION_ROTATION);
    const __m128i* placement = reinterpret_cast<const __m128i*>(motion + OFF_MOTION_CENTER_OF_MASS);
    __m128i* out = reinterpret_cast<__m128i*>(&pose);

    _mm_storeu_si128(out, _mm_loadu_si128(rotation));

    for (int i = 0; i < 5; i++)
    {
        _mm_storeu_si128(out + 1 + i, _mm_loadu_si128(placement + i));
    }
}

static void WritePose(uintptr_t motion, const BodyPose& pose)
{
    __m128i* rotation = reinterpret_cast<__m128i*>(motion + OFF_MOTION_ROTATION);
    __m128i* placement = reinterpret_cast<__m128i*>(motion + OFF_MOTION_CENTER_OF_MASS);
    const __m128i* in = reinterpret_cast<const __m128i*>(&pose);

    _mm_storeu_si128(rotation, _mm_loadu_si128(in));

    for (int i = 0; i < 5; i++)
    {
        _mm_storeu_si128(placement + i, _mm_loadu_si128(in + 1 + i));
    }
}

static bool HasPose(uintptr_t motion, const BodyPose& pose)
{
    const __m128i* rotation = reinterpret_cast<const __m128i*>(motion + OFF_MOTION_ROTATION);
    const __m128i* placement = reinterpret_cast<const __m128i*>(motion + OFF_MOTION_CENTER_OF_MASS);
    const __m128i* in = reinterpret_cast<const __m128i*>(&pose);

    __m128i equal = _mm_cmpeq_epi32(_mm_loadu_si128(rotation), _mm_loadu_si128(in));

    for (int i = 0; i < 5; i++)
    {
        equal = _mm_and_si128(equal, _mm_cmpeq_epi32(_mm_loadu_si128(placement + i), _mm_loadu_si128(in + 1 + i)));
    }

    return _mm_movemask_epi8(equal) == 0xFFFF;
}

static bool IsResting(const BodyPose& a, const BodyPose& b)
{
    const __m128i* pa = reinterpret_cast<const __m128i*>(&a);
    const __m128i* pb = reinterpret_cast<const __m128i*>(&b);

    __m128i rotation = _mm_cmpeq_epi32(_mm_loadu_si128(pa), _mm_loadu_si128(pb));
    __m128i centerOfMass = _mm_cmpeq_epi32(_mm_loadu_si128(pa + 1), _mm_loadu_si128(pb + 1));
    return _mm_movemask_epi8(_mm_and_si128(rotation, centerOfMass)) == 0xFFFF;
}

static bool IsFinitePose(const BodyPose& pose)
{
    const __m128i exponent = _mm_set1_epi32(0x7F800000);
    const __m128i* in = reinterpret_cast<const __m128i*>(&pose);
    __m128i notFinite = _mm_setzero_si128();

    for (int i = 0; i < 6; i++)
    {
        notFinite = _mm_or_si128(notFinite, _mm_cmpeq_epi32(_mm_and_si128(_mm_loadu_si128(in + i), exponent), exponent));
    }

    return _mm_movemask_epi8(notFinite) == 0;
}

static __m128 Nlerp(const __m128& a, const __m128& b, float t)
{
    // Shortest path between the two rotations
    __m128 sign = _mm_set1_ps(SumInOrder(_mm_mul_ps(a, b)) < 0.0f ? -1.0f : 1.0f);
    __m128 q = _mm_add_ps(a, _mm_mul_ps(_mm_sub_ps(_mm_mul_ps(b, sign), a), _mm_set1_ps(t)));

    __m128 length = _mm_sqrt_ss(_mm_set_ss(SumInOrder(_mm_mul_ps(q, q))));
    if (_mm_cvtss_f32(length) < 1e-6f) return b;

    __m128 invLength = _mm_div_ss(_mm_set_ss(1.0f), length);
    return _mm_mul_ps(q, Splat<0>(invLength));
}

// Same as hkRotation::set from a quaternion
static void SetRotation(const __m128& q, __m128& column0, __m128& column1, __m128& column2)
{
    __m128 q2 = _mm_add_ps(q, q);
    __m128 px = _mm_mul_ps(q2, Splat<0>(q)); // xx xy xz
    __m128 py = _mm_mul_ps(q2, Splat<1>(q)); // -  yy yz
    __m128 pz = _mm_mul_ps(q2, Splat<2>(q)); // -  -  zz
    __m128 pw = _mm_mul_ps(q2, Splat<3>(q)); // wx wy wz

    __m128 products = _mm_shuffle_ps(px, py, _MM_SHUFFLE(2, 2, 2, 1)); // xy xz yz
    __m128 rotations = _mm_shuffle_ps(pw, pw, _MM_SHUFFLE(0, 0, 1, 2)); // wz wy wx
    __m128 sums = _mm_and_ps(_mm_add_ps(products, rotations), MaskXYZ());
    __m128 differences = _mm_and_ps(_mm_sub_ps(products, rotations), MaskXYZ());

    __m128 squares = _mm_unpacklo_ps(px, py);
    __m128 first = _mm_shuffle_ps(pz, py, _MM_SHUFFLE(1, 1, 2, 2)); // zz zz yy
    __m128 second = _mm_shuffle_ps(squares, squares, _MM_SHUFFLE(0, 0, 0, 3)); // yy xx xx
    __m128 diagonal = _mm_and_ps(_mm_sub_ps(_mm_set1_ps(1.0f), _mm_add_ps(first, second)), MaskXYZ());

    column0 = _mm_shuffle_ps(_mm_unpacklo_ps(diagonal, sums), differences, _MM_SHUFFLE(3, 1, 1, 0));
    column1 = _mm_shuffle_ps(_mm_unpacklo_ps(differences, diagonal), sums, _MM_SHUFFLE(3, 2, 3, 0));
    column2 = _mm_shuffle_ps(_mm_shuffle_ps(sums, differences, _MM_SHUFFLE(2, 2, 1, 1)), diagonal, _MM_SHUFFLE(3, 2, 2, 0));
}

// Only x, y and z are meaningful
static __m128 Rotate(const __m128& column0, const __m128& column1, const __m128& column2, const __m128& v)
{
    __m128 x = _mm_mul_ps(column0, Splat<0>(v));
    __m128 y = _mm_mul_ps(column1, Splat<1>(v));
    __m128 z = _mm_mul_ps(column2, Splat<2>(v));
    return _mm_add_ps(_mm_add_ps(x, y), z);
}

// (x + y) + z of the squared differences
static float GetSquaredDistance(const HkVector4& from, const HkVector4& to)
{
    __m128 moved = _mm_sub_ps(LoadVector(to), LoadVector(from));
    moved = _mm_mul_ps(moved, moved);
    return _mm_cvtss_f32(_mm_add_ss(_mm_add_ss(moved, Splat<1>(moved)), _mm_movehl_ps(moved, moved)));
}

// How fast its farthest point moves, both lengths in one square root
static float GetSpeed(uintptr_t motion)
{
    __m128 v = LoadVector(motion + OFF_MOTION_LINEAR_VELOCITY);
    __m128 w = LoadVector(motion + OFF_MOTION_ANGULAR_VELOCITY);
    v = _mm_mul_ps(v, v);
    w = _mm_mul_ps(w, w);

    __m128 low = _mm_unpacklo_ps(v, w);
    __m128 lengths = _mm_sqrt_ps(_mm_add_ps(_mm_add_ps(low, _mm_movehl_ps(low, low)), _mm_unpackhi_ps(v, w)));
    return _mm_cvtss_f32(lengths) + _mm_cvtss_f32(Splat<1>(lengths)) * LIMB_RADIUS;
}

static void BuildInterpolatedPose(uintptr_t motion, const SimulatedBody& body, float alpha, BodyPose& pose)
{
    const BodyPose& a = body.previous;
    const BodyPose& b = body.current;

    __m128 previousCenter = LoadVector(a.centerOfMass);
    __m128 currentCenter = LoadVector(b.centerOfMass);
    __m128 centerOfMass = _mm_add_ps(previousCenter, _mm_mul_ps(_mm_sub_ps(currentCenter, previousCenter), _mm_set1_ps(alpha)));
    centerOfMass = SelectXYZ(centerOfMass, currentCenter);
    __m128 rotation = Nlerp(LoadVector(a.rotation), LoadVector(b.rotation), alpha);

    // Placed by its center of mass, like the integration does
    __m128 column0, column1, column2;
    SetRotation(rotation, column0, column1, column2);
    __m128 offset = Rotate(column0, column1, column2, LoadVector(motion + OFF_MOTION_CENTER_OF_MASS_LOCAL));
    __m128 translation = SelectXYZ(_mm_sub_ps(centerOfMass, offset), LoadVector(b.transform[3]));

    StoreVector(pose.rotation, rotation);
    StoreVector(pose.centerOfMass, centerOfMass);
    StoreVector(pose.transform[0], column0);
    StoreVector(pose.transform[1], column1);
    StoreVector(pose.transform[2], column2);
    StoreVector(pose.transform[3], translation);
}

static void BuildKeyframedPose(uintptr_t motion, const float* transform, BodyPose& pose)
{
    __m128 rotation = _mm_loadu_ps(transform + 3);
    __m128 column0, column1, column2;
    SetRotation(rotation, column0, column1, column2);

    __m128 position = _mm_mul_ps(_mm_loadu_ps(transform), _mm_set1_ps(GAME_TO_HAVOK_SCALE));
    __m128 translation = SelectXYZ(position, LoadVector(motion + OFF_MOTION_TRANSFORM + 3 * sizeof(HkVector4)));
    __m128 offset = Rotate(column0, column1, column2, LoadVector(motion + OFF_MOTION_CENTER_OF_MASS_LOCAL));
    __m128 centerOfMass = SelectXYZ(_mm_add_ps(translation, offset), LoadVector(motion + OFF_MOTION_CENTER_OF_MASS));

    StoreVector(pose.rotation, rotation);
    StoreVector(pose.centerOfMass, centerOfMass);
    StoreVector(pose.transform[0], column0);
    StoreVector(pose.transform[1], column1);
    StoreVector(pose.transform[2], column2);
    StoreVector(pose.transform[3], translation);
}

// =========================
// Physics Worlds
// =========================

static PhysicsWorld* FindPhysicsWorld(uintptr_t world)
{
    if (!world) return nullptr;

    for (PhysicsWorld& physicsWorld : s_physicsWorlds)
    {
        if (physicsWorld.world == world) return &physicsWorld;
    }

    return nullptr;
}

static PhysicsWorld* FindEntityPhysicsWorld(uintptr_t entity)
{
    return entity ? FindPhysicsWorld(GetEntityWorld(entity)) : nullptr;
}

static void ForgetPhysicsWorld(PhysicsWorld& physicsWorld)
{
    uintptr_t world = physicsWorld.world;

    s_simulatedBodies.EraseIf([world](const SimulatedBody& body) { return body.world == world; });
    s_keyframeRequests.EraseIf([world](const KeyframeRequest& request) { return request.world == world; });

    physicsWorld = {};
}

static PhysicsWorld* GetPhysicsWorld(uintptr_t sim, uintptr_t world)
{
    PhysicsWorld* freeWorld = nullptr;

    for (PhysicsWorld& physicsWorld : s_physicsWorlds)
    {
        if (physicsWorld.sim == sim)
        {
            // The level changed
            if (physicsWorld.world != world)
            {
                ForgetPhysicsWorld(physicsWorld);
                physicsWorld.sim = sim;
                physicsWorld.world = world;
            }

            return &physicsWorld;
        }

        if (!freeWorld && !physicsWorld.sim) freeWorld = &physicsWorld;
    }

    if (freeWorld)
    {
        freeWorld->sim = sim;
        freeWorld->world = world;
    }

    return freeWorld;
}

// =========================
// Keyframes
// =========================

static void ClearKeyframeRequests(const PhysicsWorld& physicsWorld)
{
    uintptr_t world = physicsWorld.world;
    s_keyframeRequests.EraseIf([world](const KeyframeRequest& request) { return request.world == world; });
}

static __declspec(noinline) void CallSoftKeyframe(uintptr_t entity, const HkVector4* position, const HkVector4* rotation, float invDeltaTime, float weight, float maxLinearVelocity, float maxAngularVelocity)
{
    void* softKeyframe = RigidBodySoftKeyframe;

    __asm
    {
        push edi
        push maxAngularVelocity
        push maxLinearVelocity
        push weight
        push invDeltaTime
        push rotation
        push position
        mov edi, entity
        call softKeyframe
        add esp, 24
        pop edi
    }
}

static void ApplyKeyframeRequests(const PhysicsWorld& physicsWorld, float invDeltaTime)
{
    if (s_keyframeRequests.Size() == 0) return;

    // Applying one can wake its island up, so not while going through the islands
    s_keyframedEntities.clear();

    ForEachWorldEntity(physicsWorld.world, [](uintptr_t entity)
    {
        if (s_keyframeRequests.Find(entity)) s_keyframedEntities.push_back(entity);
    });

    for (uintptr_t entity : s_keyframedEntities)
    {
        const KeyframeRequest* found = s_keyframeRequests.Find(entity);
        if (!found || found->world != physicsWorld.world) continue;

        const KeyframeRequest& request = *found;

        // Made dynamic or keyframed since, by a ragdoll for one
        if (GetMotion(entity) != request.motion) continue;

        if (request.isSoft)
        {
            CallSoftKeyframe(entity, &request.position, &request.rotation, invDeltaTime, request.weight, request.maxLinearVelocity, request.maxAngularVelocity);
        }
        else if (GetEntity(request.rigidBody) == entity)
        {
            RigidBodyKeyframe(request.rigidBody, request.transform, invDeltaTime);
        }
    }
}

// Gives the ragdoll the velocity the game's keyframe of this frame would have given it before
static void HandOverKeyframe(const PhysicsWorld& physicsWorld, uintptr_t rigidBody, uintptr_t entity)
{
    uintptr_t motion = GetMotion(entity);
    if (GetMotionType(motion) != MOTION_KEYFRAMED) return;

    const KeyframeRequest* request = s_keyframeRequests.Find(entity);
    if (!request || request->isSoft || request->motion != motion || request->frame != physicsWorld.frame) return;

    // It has to be where the previous frame's keyframe took it
    const SimulatedBody* body = s_simulatedBodies.Find(entity);
    bool isShown = body && body->isShown && HasPose(motion, body->shown);
    if (!isShown && !physicsWorld.lastUpdateTicked) return;

    // The game's own time is in whole milliseconds, too coarse at high framerates
    float invDeltaTime = physicsWorld.frameTime > 0.0f ? 1.0f / physicsWorld.frameTime : request->invDeltaTime;

    RigidBodyKeyframe(rigidBody, request->transform, invDeltaTime);
}

// =========================
// Interpolation
// =========================

static void RestoreBodies(PhysicsWorld& physicsWorld)
{
    if (physicsWorld.shownBodies == 0) return;

    auto restore = [&physicsWorld](uintptr_t entity)
    {
        SimulatedBody* found = s_simulatedBodies.Find(entity);
        if (!found || !found->isShown) return true;

        SimulatedBody& body = *found;
        uintptr_t motion = GetMotion(entity);

        // Keep the pose the game gave the body since
        if (motion == body.motion && HasPose(motion, body.shown))
        {
            WritePose(motion, body.restore);
        }
        else
        {
            body.previousTick = 0;
        }

        body.isShown = false;
        physicsWorld.shownBodies--;
        return physicsWorld.shownBodies != 0;
    };

    if (ForEachIslandEntityWhile(physicsWorld.world, OFF_WORLD_ACTIVE_ISLANDS, restore))
    {
        ForEachIslandEntityWhile(physicsWorld.world, OFF_WORLD_INACTIVE_ISLANDS, restore);
    }

    // Removed from the world while shown
    if (physicsWorld.shownBodies != 0)
    {
        uintptr_t world = physicsWorld.world;
        s_simulatedBodies.ForEach([world](SimulatedBody& body) { if (body.world == world) body.isShown = false; });

        physicsWorld.shownBodies = 0;
    }
}

static void CapturePreviousPoses(const PhysicsWorld& physicsWorld)
{
    ForEachIslandEntity(physicsWorld.world, OFF_WORLD_ACTIVE_ISLANDS, [&physicsWorld](uintptr_t entity)
    {
        uintptr_t motion = GetMotion(entity);
        if (!IsDynamicMotion(GetMotionType(motion))) return;

        SimulatedBody& body = s_simulatedBodies[entity];
        body.world = physicsWorld.world;
        body.motion = motion;
        body.previousTick = physicsWorld.tick;
        ReadPose(motion, body.previous);
    });
}

static void CaptureCurrentPoses(const PhysicsWorld& physicsWorld)
{
    ForEachIslandEntity(physicsWorld.world, OFF_WORLD_ACTIVE_ISLANDS, [&physicsWorld](uintptr_t entity)
    {
        uintptr_t motion = GetMotion(entity);
        if (!IsDynamicMotion(GetMotionType(motion))) return;

        SimulatedBody& body = s_simulatedBodies[entity];
        ReadPose(motion, body.current);

        // Woken up during the tick or given a new motion, nothing to interpolate from
        if (body.world != physicsWorld.world || body.motion != motion || body.previousTick != physicsWorld.tick)
        {
            body.world = physicsWorld.world;
            body.motion = motion;
            body.previous = body.current;
            body.previousTick = physicsWorld.tick;
        }

        body.currentTick = physicsWorld.tick;
    });

    uintptr_t world = physicsWorld.world;
    uint32_t tick = physicsWorld.tick;
    s_simulatedBodies.EraseIf([world, tick](const SimulatedBody& body) { return body.world == world && body.currentTick != tick && !body.isShown; });
}

static SimulatedBody* ShowKeyframedBody(const PhysicsWorld& physicsWorld, uintptr_t entity, uintptr_t motion)
{
    const KeyframeRequest* request = s_keyframeRequests.Find(entity);
    if (!request || request->isSoft || request->world != physicsWorld.world || request->motion != motion) return nullptr;

    // Where the game wants it now, the next tick moves it there
    SimulatedBody& body = s_simulatedBodies[entity];
    body.world = physicsWorld.world;
    body.motion = motion;
    body.previousTick = 0;
    ReadPose(motion, body.restore);
    BuildKeyframedPose(motion, request->transform, body.shown);

    return &body;
}

static SimulatedBody* ShowSimulatedBody(const PhysicsWorld& physicsWorld, uintptr_t entity, uintptr_t motion)
{
    SimulatedBody* found = s_simulatedBodies.Find(entity);
    if (!found) return nullptr;

    SimulatedBody& body = *found;
    if (body.world != physicsWorld.world || body.motion != motion || body.previousTick != physicsWorld.tick || body.currentTick != physicsWorld.tick) return nullptr;

    if (IsResting(body.previous, body.current)) return nullptr;

    // Teleported rather than simulated
    if (GetSquaredDistance(body.previous.centerOfMass, body.current.centerOfMass) > SNAP_DISTANCE * SNAP_DISTANCE) return nullptr;

    // Moved by the game since the tick
    if (!HasPose(motion, body.current))
    {
        body.previousTick = 0;
        return nullptr;
    }

    body.restore = body.current;
    BuildInterpolatedPose(motion, body, physicsWorld.alpha, body.shown);

    return &body;
}

static void ShowBodies(PhysicsWorld& physicsWorld, bool updateOwners)
{
    s_movedOwners.clear();

    // Asleep, a body has nothing to show until its keyframe wakes it up at the next tick
    ForEachIslandEntity(physicsWorld.world, OFF_WORLD_ACTIVE_ISLANDS, [&physicsWorld, updateOwners](uintptr_t entity)
    {
        uintptr_t motion = GetMotion(entity);
        int type = GetMotionType(motion);
        SimulatedBody* body = nullptr;

        if (type == MOTION_KEYFRAMED)
        {
            body = ShowKeyframedBody(physicsWorld, entity, motion);
        }
        else if (IsDynamicMotion(type))
        {
            body = ShowSimulatedBody(physicsWorld, entity, motion);
        }

        if (!body || !IsFinitePose(body->shown)) return;

        WritePose(motion, body->shown);
        body->isShown = true;
        physicsWorld.shownBodies++;

        if (!updateOwners) return;

        if (uintptr_t rigidBody = GetRigidBody(entity))
        {
            RigidBodyMarkTransformDirty(rigidBody);

            if (uintptr_t owner = *reinterpret_cast<uintptr_t*>(rigidBody + OFF_RIGID_BODY_OWNER))
            {
                s_movedOwners.push_back(owner);
            }
        }
    });

    if (s_movedOwners.empty()) return;

    std::sort(s_movedOwners.begin(), s_movedOwners.end());
    s_movedOwners.erase(std::unique(s_movedOwners.begin(), s_movedOwners.end()), s_movedOwners.end());

    // Same as after the game's own physics steps
    using OnPhysicsUpdate = void(__thiscall*)(uintptr_t);

    for (uintptr_t owner : s_movedOwners)
    {
        (*reinterpret_cast<OnPhysicsUpdate**>(owner))[VT_OWNER_ON_PHYSICS_UPDATE](owner);
        ObjectMarkAttachmentsDirty(owner);
    }
}

static void ForgetBody(uintptr_t entity)
{
    if (SimulatedBody* found = s_simulatedBodies.Find(entity))
    {
        SimulatedBody& body = *found;

        if (body.isShown)
        {
            uintptr_t motion = GetMotion(entity);

            if (motion == body.motion && HasPose(motion, body.shown))
            {
                WritePose(motion, body.restore);
            }

            if (PhysicsWorld* physicsWorld = FindPhysicsWorld(body.world))
            {
                physicsWorld->shownBodies = std::max(physicsWorld->shownBodies - 1, 0);
            }
        }

        s_simulatedBodies.Erase(entity);
    }

    s_keyframeRequests.Erase(entity);
}

// =========================
// Substeps
// =========================

static int ChooseSubsteps(const PhysicsWorld& physicsWorld)
{
    float fastest = 0.0f;
    int bodies = 0;

    ForEachIslandEntity(physicsWorld.world, OFF_WORLD_ACTIVE_ISLANDS, [&fastest, &bodies](uintptr_t entity)
    {
        uintptr_t motion = GetMotion(entity);
        if (!IsDynamicMotion(GetMotionType(motion))) return;

        fastest = std::max(fastest, GetSpeed(motion));

        bodies++;
    });

    int substeps = 1;
    while (substeps < MAX_SUBSTEPS && fastest * physicsWorld.tickTime > SUBSTEP_TRAVEL * substeps) substeps *= 2;

    // Too costly with many bodies awake, as after loading a level, and a frame catching up on ticks would only get slower
    while (substeps > 1 && (bodies * substeps > SUBSTEP_BODY_BUDGET || physicsWorld.ticksThisFrame > MAX_SUBSTEPPED_TICKS_PER_FRAME)) substeps /= 2;

    // Each substep gets its share of the solver steps, so they keep the length they have at 60Hz
    int solverSteps = *reinterpret_cast<int*>(physicsWorld.world + OFF_WORLD_SOLVER_STEPS);
    while (substeps > 1 && (solverSteps < substeps || solverSteps % substeps != 0)) substeps /= 2;

    return substeps;
}

static int StepWorld(const PhysicsWorld& physicsWorld)
{
    uintptr_t world = physicsWorld.world;
    int substeps = ChooseSubsteps(physicsWorld);

    if (substeps == 1)
    {
        return hkWorldStepDeltaTime(world, physicsWorld.tickTime);
    }

    int& solverSteps = *reinterpret_cast<int*>(world + OFF_WORLD_SOLVER_STEPS);
    float& invSolverSteps = *reinterpret_cast<float*>(world + OFF_WORLD_SOLVER_INV_STEPS);
    float& queryInvSolverSteps = *reinterpret_cast<float*>(world + OFF_WORLD_QUERY_INV_STEPS);

    int savedSolverSteps = solverSteps;
    float savedInvSolverSteps = invSolverSteps;
    float savedQueryInvSolverSteps = queryInvSolverSteps;

    solverSteps = savedSolverSteps / substeps;
    invSolverSteps = 1.0f / solverSteps;
    queryInvSolverSteps = invSolverSteps;

    int result = 0;

    for (int i = 0; i < substeps; i++)
    {
        result = hkWorldStepDeltaTime(world, physicsWorld.tickTime / substeps);
    }

    solverSteps = savedSolverSteps;
    invSolverSteps = savedInvSolverSteps;
    queryInvSolverSteps = savedQueryInvSolverSteps;

    return result;
}

// =========================
// Time Scale
// =========================

static void UpdateTickTime(PhysicsWorld& physicsWorld, float frameTime)
{
    if (s_counterFrequency <= 0.0) return;

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    if (physicsWorld.lastCounter == 0)
    {
        physicsWorld.lastCounter = counter.QuadPart;
        return;
    }

    if (!(frameTime > 0.0f)) return;

    double realTime = static_cast<double>(counter.QuadPart - physicsWorld.lastCounter) / s_counterFrequency;
    physicsWorld.lastCounter = counter.QuadPart;

    if (!(realTime > 0.0) || realTime > MAX_TIME_SCALE_SAMPLE) return;

    auto dropOldest = [&physicsWorld]()
    {
        const TimeScaleSample& oldest = physicsWorld.timeScaleSamples[physicsWorld.firstTimeScaleSample];
        physicsWorld.scaledTimeSum -= oldest.scaledTime;
        physicsWorld.realTimeSum -= oldest.realTime;
        physicsWorld.firstTimeScaleSample = (physicsWorld.firstTimeScaleSample + 1) % TIME_SCALE_SAMPLES;
        physicsWorld.timeScaleSampleCount--;
    };

    if (physicsWorld.timeScaleSampleCount == TIME_SCALE_SAMPLES) dropOldest();

    int last = (physicsWorld.firstTimeScaleSample + physicsWorld.timeScaleSampleCount) % TIME_SCALE_SAMPLES;
    physicsWorld.timeScaleSamples[last] = { frameTime, static_cast<float>(realTime) };
    physicsWorld.timeScaleSampleCount++;
    physicsWorld.scaledTimeSum += frameTime;
    physicsWorld.realTimeSum += static_cast<float>(realTime);

    while (physicsWorld.timeScaleSampleCount > 1 && physicsWorld.realTimeSum - physicsWorld.timeScaleSamples[physicsWorld.firstTimeScaleSample].realTime >= TIME_SCALE_WINDOW)
    {
        dropOldest();
    }

    float timeScale = static_cast<float>(physicsWorld.scaledTimeSum / physicsWorld.realTimeSum);

    if (fabsf(timeScale - 1.0f) < TIME_SCALE_SNAP) timeScale = 1.0f;

    physicsWorld.tickTime = std::clamp(TARGET_FRAME_TIME * timeScale, MIN_TICK_TIME, TARGET_FRAME_TIME);
}

// =========================
// HavokPhysicsFix
// =========================

static void __fastcall PhysicsSimUpdate_Hook(uintptr_t sim, int, float frameTime, float targetFrameTime, unsigned int maxSteps)
{
    uintptr_t world = *reinterpret_cast<uintptr_t*>(sim + OFF_SIM_WORLD);
    PhysicsWorld* physicsWorld = world ? GetPhysicsWorld(sim, world) : nullptr;

    if (!physicsWorld || physicsWorld->isUpdating)
    {
        PhysicsSimUpdate(sim, frameTime, targetFrameTime, maxSteps);
        return;
    }

    RestoreBodies(*physicsWorld);
    UpdateTickTime(*physicsWorld, frameTime);

    if (frameTime > 0.0f)
    {
        physicsWorld->frameTime = frameTime;
        physicsWorld->accumulator += frameTime;
    }

    double tickTime = physicsWorld->tickTime;
    int ticks = static_cast<int>((physicsWorld->accumulator + TICK_EARLINESS) / tickTime);

    // Too far behind, the time that can't be caught up on is dropped
    if (ticks > MAX_TICKS_PER_FRAME)
    {
        physicsWorld->accumulator -= (ticks - MAX_TICKS_PER_FRAME) * tickTime;
        ticks = MAX_TICKS_PER_FRAME;
    }

    physicsWorld->accumulator -= ticks * tickTime;
    physicsWorld->alpha = std::clamp(static_cast<float>(physicsWorld->accumulator / tickTime), 0.0f, 1.0f);

    if (ticks == 0)
    {
        ShowBodies(*physicsWorld, true);
    }
    else
    {
        // The game's own update runs the ticks as its steps, then updates its objects from the bodies
        physicsWorld->ticksThisFrame = ticks;
        physicsWorld->tickIndex = 0;
        physicsWorld->isUpdating = true;
        PhysicsSimUpdate(sim, physicsWorld->tickTime * ticks, physicsWorld->tickTime, ticks);
        physicsWorld->isUpdating = false;

        // It stepped differently than asked, show what was simulated
        if (physicsWorld->tickIndex != ticks)
        {
            if (physicsWorld->tickIndex > 0)
            {
                ClearKeyframeRequests(*physicsWorld);
                CaptureCurrentPoses(*physicsWorld);
            }

            ShowBodies(*physicsWorld, true);
        }
    }

    physicsWorld->frame++;
    physicsWorld->lastUpdateTicked = ticks > 0;
}

static int __fastcall PhysicsSimTerm_Hook(uintptr_t sim, int)
{
    for (PhysicsWorld& physicsWorld : s_physicsWorlds)
    {
        if (physicsWorld.sim == sim) ForgetPhysicsWorld(physicsWorld);
    }

    return PhysicsSimTerm(sim);
}

static int __fastcall hkWorldStepDeltaTime_Hook(uintptr_t world, int, float deltaTime)
{
    PhysicsWorld* physicsWorld = FindPhysicsWorld(world);

    if (!physicsWorld || !physicsWorld->isUpdating)
    {
        if (physicsWorld) RestoreBodies(*physicsWorld);
        return hkWorldStepDeltaTime(world, deltaTime);
    }

    int remainingTicks = std::max(physicsWorld->ticksThisFrame - physicsWorld->tickIndex, 1);
    bool isLastTick = remainingTicks == 1;

    // The game keyframed once for the whole frame, its ticks share the way to the target
    ApplyKeyframeRequests(*physicsWorld, 1.0f / (physicsWorld->tickTime * remainingTicks));

    physicsWorld->tick++;
    if (isLastTick) CapturePreviousPoses(*physicsWorld);

    physicsWorld->isStepping = true;
    int result = StepWorld(*physicsWorld);
    physicsWorld->isStepping = false;
    physicsWorld->tickIndex++;

    if (isLastTick)
    {
        ClearKeyframeRequests(*physicsWorld);
        CaptureCurrentPoses(*physicsWorld);

        // Shown before the game updates its objects from the bodies
        ShowBodies(*physicsWorld, false);
    }

    return result;
}

static uint8_t __fastcall RigidBodyKeyframe_Hook(uintptr_t rigidBody, int, const float* transform, float invDeltaTime)
{
    uintptr_t entity = GetEntity(rigidBody);
    PhysicsWorld* physicsWorld = FindEntityPhysicsWorld(entity);

    if (!physicsWorld || physicsWorld->isStepping)
    {
        return RigidBodyKeyframe(rigidBody, transform, invDeltaTime);
    }

    // The velocity it sets is for one frame, but the frames between ticks aren't simulated
    KeyframeRequest& request = s_keyframeRequests[entity];
    request = {};
    request.world = physicsWorld->world;
    request.rigidBody = rigidBody;
    request.motion = GetMotion(entity);
    request.frame = physicsWorld->frame;
    request.invDeltaTime = invDeltaTime;
    memcpy(request.transform, transform, sizeof(request.transform));

    return 1;
}

static void __cdecl RigidBodySoftKeyframe_Handler(uintptr_t entity, const HkVector4* position, const HkVector4* rotation, float invDeltaTime, float weight, float maxLinearVelocity, float maxAngularVelocity)
{
    PhysicsWorld* physicsWorld = FindEntityPhysicsWorld(entity);

    if (!physicsWorld || physicsWorld->isStepping)
    {
        CallSoftKeyframe(entity, position, rotation, invDeltaTime, weight, maxLinearVelocity, maxAngularVelocity);
        return;
    }

    // Blends the velocity toward the target on each call, so once per tick
    KeyframeRequest& request = s_keyframeRequests[entity];
    request = {};
    request.world = physicsWorld->world;
    request.motion = GetMotion(entity);
    request.frame = physicsWorld->frame;
    request.position = *position;
    request.rotation = *rotation;
    request.invDeltaTime = invDeltaTime;
    request.weight = weight;
    request.maxLinearVelocity = maxLinearVelocity;
    request.maxAngularVelocity = maxAngularVelocity;
    request.isSoft = true;
}

static __declspec(naked) void RigidBodySoftKeyframe_Hook()
{
    __asm
    {
        push dword ptr [esp + 24]
        push dword ptr [esp + 24]
        push dword ptr [esp + 24]
        push dword ptr [esp + 24]
        push dword ptr [esp + 24]
        push dword ptr [esp + 24]
        push edi
        call RigidBodySoftKeyframe_Handler
        add esp, 28
        ret
    }
}

static int __fastcall RigidBodyTerm_Hook(uintptr_t rigidBody, int)
{
    if (uintptr_t entity = GetEntity(rigidBody))
    {
        ForgetBody(entity);
    }

    return RigidBodyTerm(rigidBody);
}

static void __fastcall RigidBodySetKeyframed_Hook(uintptr_t rigidBody, int, bool keyframed)
{
    uintptr_t entity = GetEntity(rigidBody);
    PhysicsWorld* physicsWorld = FindEntityPhysicsWorld(entity);
    uintptr_t motion = entity ? GetMotion(entity) : 0;

    // Made a ragdoll
    if (physicsWorld && !physicsWorld->isStepping && !keyframed)
    {
        HandOverKeyframe(*physicsWorld, rigidBody, entity);
    }

    RigidBodySetKeyframed(rigidBody, keyframed);

    // Its keyframes were for the motion it had
    if (entity && GetMotion(entity) != motion)
    {
        s_keyframeRequests.Erase(entity);
    }
}

// A force applies until the next tick, which can be several frames away
static float GetForceScale(uintptr_t rigidBody)
{
    PhysicsWorld* physicsWorld = FindEntityPhysicsWorld(GetEntity(rigidBody));
    if (!physicsWorld || physicsWorld->isStepping || !(physicsWorld->frameTime > 0.0f)) return 1.0f;

    return std::min(physicsWorld->frameTime / physicsWorld->tickTime, static_cast<float>(MAX_TICKS_PER_FRAME));
}

static int __fastcall RigidBodyApplyForce_Hook(uintptr_t rigidBody, int, const float* force, const float* point)
{
    float scale = GetForceScale(rigidBody);
    float scaledForce[3] = { force[0] * scale, force[1] * scale, force[2] * scale };

    return RigidBodyApplyForce(rigidBody, scaledForce, point);
}

static int __fastcall RigidBodyApplyTorque_Hook(uintptr_t rigidBody, int, const float* torque)
{
    float scale = GetForceScale(rigidBody);
    float scaledTorque[3] = { torque[0] * scale, torque[1] * scale, torque[2] * scale };

    return RigidBodyApplyTorque(rigidBody, scaledTorque);
}

static void ApplyHavokPhysicsFix()
{
    if (!HavokPhysicsFix) return;

    LARGE_INTEGER frequency;
    if (QueryPerformanceFrequency(&frequency)) s_counterFrequency = static_cast<double>(frequency.QuadPart);

    RigidBodyMarkTransformDirty = reinterpret_cast<decltype(RigidBodyMarkTransformDirty)>(GetAddress(Addr::RigidBodyMarkTransformDirty));
    ObjectMarkAttachmentsDirty = reinterpret_cast<decltype(ObjectMarkAttachmentsDirty)>(GetAddress(Addr::ObjectMarkAttachmentsDirty));

    HookHelper::ApplyHook((void*)GetAddress(Addr::PhysicsSimUpdate), &PhysicsSimUpdate_Hook, (LPVOID*)&PhysicsSimUpdate, g_State.CurrentFEARGame == FEAR);
    HookHelper::ApplyHook((void*)GetAddress(Addr::PhysicsSimTerm), &PhysicsSimTerm_Hook, (LPVOID*)&PhysicsSimTerm, g_State.CurrentFEARGame == FEAR);
    HookHelper::ApplyHook((void*)GetAddress(Addr::hkWorldStepDeltaTime), &hkWorldStepDeltaTime_Hook, (LPVOID*)&hkWorldStepDeltaTime, g_State.CurrentFEARGame == FEAR);
    HookHelper::ApplyHook((void*)GetAddress(Addr::RigidBodyKeyframe), &RigidBodyKeyframe_Hook, (LPVOID*)&RigidBodyKeyframe, g_State.CurrentFEARGame == FEAR);
    HookHelper::ApplyHook((void*)GetAddress(Addr::RigidBodySoftKeyframe), &RigidBodySoftKeyframe_Hook, (LPVOID*)&RigidBodySoftKeyframe, g_State.CurrentFEARGame == FEAR);
    HookHelper::ApplyHook((void*)GetAddress(Addr::RigidBodyTerm), &RigidBodyTerm_Hook, (LPVOID*)&RigidBodyTerm, g_State.CurrentFEARGame == FEAR);
    HookHelper::ApplyHook((void*)GetAddress(Addr::RigidBodySetKeyframed), &RigidBodySetKeyframed_Hook, (LPVOID*)&RigidBodySetKeyframed, g_State.CurrentFEARGame == FEAR);
    HookHelper::ApplyHook((void*)GetAddress(Addr::RigidBodyApplyForce), &RigidBodyApplyForce_Hook, (LPVOID*)&RigidBodyApplyForce, g_State.CurrentFEARGame == FEAR);
    HookHelper::ApplyHook((void*)GetAddress(Addr::RigidBodyApplyTorque), &RigidBodyApplyTorque_Hook, (LPVOID*)&RigidBodyApplyTorque, g_State.CurrentFEARGame == FEAR);
}