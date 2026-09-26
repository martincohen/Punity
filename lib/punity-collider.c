#define COLLISION_EPSILON (0.001f)

#define USE_FIXED 1

#if USE_FIXED
#define FIXED_SCALE (8)
#define FIXED_SCALE_HALF (FIXED_SCALE/2)
#define FIXED_TYPE int
#define TO_FIXED_(n) ((n) * FIXED_SCALE)
#define FIXED_DIV_(a, b) ((((a) + ((b)/2)) * FIXED_SCALE)/(b))
#define FIXED_MUL_(a, b) (((a) * (b) + FIXED_SCALE_HALF) / FIXED_SCALE)
#define TO_FLOAT_(n) ((f32)(n) / (f32)FIXED_SCALE)
#define FIXED_EQUAL(n, x) ((n) == (x))
#else
#define FIXED_TYPE f32
#define TO_FIXED_(n) (n)
#define FIXED_DIV_(a, b) ((a)/(b))
#define FIXED_MUL_(a, b) ((a) * (b))
#define TO_FLOAT_(n) (float)(n)
#define FIXED_EQUAL(n, x) equalf(n, x, COLLISION_EPSILON)
#endif

enum
{
    ColliderFlag_EdgeLeft       = 1 << 0,
    ColliderFlag_EdgeTop        = 1 << 1,
    ColliderFlag_EdgeRight      = 1 << 2,
    ColliderFlag_EdgeBottom     = 1 << 3,
    ColliderFlag_EdgeAll        = ColliderFlag_EdgeLeft
                                | ColliderFlag_EdgeTop
                                | ColliderFlag_EdgeRight
                                | ColliderFlag_EdgeBottom,
};

typedef enum
{
    // Collision checking continues.
    // The callback has set dx and dy members
    // that are used to continue.
    // This is useful for gliding and bouncing.
    OnCollisionResult_Continue,

    // Collision is ignored.
    // This is useful for triggers.
    // B is flagged as temporarily ignored.
    // Retry counter is reset to it's normal value.
    OnCollisionResult_Ignore,

    // Collision checking stops immediately.
    // This is useful when A dies.
    OnCollisionResult_Stop
}
OnCollisionResult;

typedef struct
{
    FIXED_TYPE x, y;
    FIXED_TYPE w, h;
    uint8_t flags;
}
Collider;

typedef struct
{
    Collider *A;
    Collider *B;
    FIXED_TYPE t;
    FIXED_TYPE normal_x;
    FIXED_TYPE normal_y;
    FIXED_TYPE end_x;
    FIXED_TYPE end_y;
    FIXED_TYPE over_x;
    FIXED_TYPE over_y;
    FIXED_TYPE delta_x;
    FIXED_TYPE delta_y;
}
Collision;

Collider
collider_make_rect(f32 min_x, f32 min_y, f32 max_x, f32 max_y, u8 flags)
{
    Collider A;
    A.w = TO_FIXED_((max_x - min_x) / 2);
    A.h = TO_FIXED_((max_y - min_y) / 2);
    A.x = TO_FIXED_(min_x) + A.w;
    A.y = TO_FIXED_(min_y) + A.h;
    A.flags = flags;
    return A;
}

bool
collider_test_overlap(Collider *A, Collider *B)
{
    // TODO:
    return false;
}

//
// Collisions
//

FIXED_TYPE
collision_test_edge_(
    FIXED_TYPE wall_a, FIXED_TYPE wall_min_b, FIXED_TYPE wall_max_b,
    FIXED_TYPE entity_a, FIXED_TYPE entity_b,
    FIXED_TYPE entity_da, FIXED_TYPE entity_db
)
{
#if USE_FIXED
    FIXED_TYPE t = FIXED_DIV_((wall_a - entity_a), entity_da);
    if (t >= 0) {
        FIXED_TYPE wall_b = entity_b + FIXED_MUL_(t, entity_db);
        if (wall_b >= wall_min_b && wall_b <= wall_max_b) {
            return t;
        }
    }
    return TO_FIXED_(1);
#else
    f32 t = (wall_a - entity_a) / entity_da;
    if (t >= 0) {
        f32 wall_b = entity_b + (t * entity_db);
        if (wall_b >= wall_min_b && wall_b <= wall_max_b) {
            return maximum(0.0f, t - 0.001f);
        }
    }
    return 1.;
#endif    
}

void
collision_init(Collision *C, Collider *A, f32 dx, f32 dy)
{
    *C = (Collision){0};
    C->A = A;
    C->end_x = A->x;
    C->end_y = A->y;
    C->delta_x = TO_FIXED_(dx);
	C->delta_y = TO_FIXED_(dy);
	C->t = TO_FIXED_(1);
}

bool
collision_finalize(Collision *C)
{
    if (C->B)
    {
        C->over_x = C->end_x + C->delta_x;
        C->over_y = C->end_y + C->delta_y;

        C->delta_x = FIXED_MUL_(C->delta_x, C->t);
        C->delta_y = FIXED_MUL_(C->delta_y, C->t);

        C->end_x += C->delta_x;
        C->end_y += C->delta_y;

        return true;
    }
    else
    {
        C->end_x += C->delta_x;
        C->end_y += C->delta_y;

        return false;
    }
}

Collision *
collision_test(Collision *C, Collider *B)
{

    FIXED_TYPE half_w = (B->w + C->A->w);
    FIXED_TYPE half_h = (B->h + C->A->h);

    FIXED_TYPE min_x = B->x - half_w;
    FIXED_TYPE min_y = B->y - half_h;
    FIXED_TYPE max_x = B->x + half_w;
    FIXED_TYPE max_y = B->y + half_h;

    FIXED_TYPE t = 0;

    if (!FIXED_EQUAL(C->delta_x, 0))
    {
        // Left edge
        if (B->flags & ColliderFlag_EdgeLeft) {
            t = collision_test_edge_(min_x, min_y, max_y,
                                     C->end_x, C->end_y, C->delta_x, C->delta_y);
            if (t < C->t) { C->t = t; C->normal_x = TO_FIXED_(-1); C->normal_y = 0; C->B = B; }
        }

        // Right edge
        if (B->flags & ColliderFlag_EdgeRight) {
            t = collision_test_edge_(max_x, min_y, max_y,
                                     C->end_x, C->end_y, C->delta_x, C->delta_y);
            if (t < C->t) { C->t = t; C->normal_x = TO_FIXED_(1); C->normal_y = 0; C->B = B; }
        }
    }

    if (!FIXED_EQUAL(C->delta_y, 0))
    {
        // Top edge
        if (B->flags & ColliderFlag_EdgeTop) {
            t = collision_test_edge_(min_y, min_x, max_x,
                                     C->end_y, C->end_x, C->delta_y, C->delta_x);
            if (t < C->t) { C->t = t; C->normal_x = 0; C->normal_y = TO_FIXED_(-1); C->B = B; }
        }

        // Bottom edge
        if (B->flags & ColliderFlag_EdgeBottom) {
            t = collision_test_edge_(max_y, min_x, max_x,
                                     C->end_y, C->end_x, C->delta_y, C->delta_x);
            if (t < C->t) { C->t = t; C->normal_x = 0; C->normal_y = TO_FIXED_(1); C->B = B; }
        }
    }

    return C;
}

OnCollisionResult
collision_bounce(Collider *A, Collider *B, Collision *C)
{
    unused(A);
    unused(B);

    // Amount of remaining movement into the wall.
    f32 dx = C->over_x - C->end_x;
    f32 dy = C->over_y - C->end_y;
    // Turn that over to a delta that is in bounce angle.
    f32 dp = v2_inner(dx, dy, C->normal_x, C->normal_y);
    C->delta_x = dx - 2 * dp * C->normal_x;
    C->delta_y = dy - 2 * dp * C->normal_y;
    return OnCollisionResult_Continue;
}

OnCollisionResult
collision_glide(Collider *A, Collider *B, Collision *C)
{
    unused(A);
    unused(B);

    // Amount of remaining movement into the wall.
    f32 dx = C->over_x - C->end_x;
    f32 dy = C->over_y - C->end_y;
    // Turn that so it goes along the wall.
    f32 dp = v2_inner(dx, dy, C->normal_x, C->normal_y);
    C->delta_x = dx - dp * C->normal_x;
    C->delta_y = dy - dp * C->normal_y;
    return OnCollisionResult_Continue;
}

OnCollisionResult
collision_stop(Collider *A, Collider *B, Collision *C)
{
    unused(A);
    unused(B);
    unused(C);

    return OnCollisionResult_Stop;
}

OnCollisionResult
collision_ignore(Collider *A, Collider *B, Collision *C)
{
    unused(A);
    unused(B);
    unused(C);

    return OnCollisionResult_Ignore;
}

void
test_collisions()
{
    Collider A = collider_make_rect(2, 2, 8, 8, ColliderFlag_EdgeAll);
    ASSERT(A.x == TO_FIXED_(5));
    ASSERT(A.y == TO_FIXED_(5));
    ASSERT(A.w == TO_FIXED_(3));
    ASSERT(A.h == TO_FIXED_(3));

	Collider B = collider_make_rect(12, 0, 20, 8, ColliderFlag_EdgeAll);

	Collision C;
	collision_init(&C, &A, 9, 0);
	collision_test(&C, &B);
    ASSERT(C.B == &B);
    if (collision_finalize(&C)) {
        // ASSERT(C.end_x == (A.x + TO_FIXED_(3)));
    }
}