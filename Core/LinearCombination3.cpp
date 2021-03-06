#include "LinearCombination3.h"
#include "RealSquareMatrices.h"

using namespace cagd;
using namespace std;

// special/default constructor
LinearCombination3::Derivatives::Derivatives(GLuint maximum_order_of_derivatives): ColumnMatrix<DCoordinate3>(maximum_order_of_derivatives + 1)
{
}

// copy constructor
LinearCombination3::Derivatives::Derivatives(const LinearCombination3::Derivatives& d): ColumnMatrix<DCoordinate3>(d)
{
}

// assignment operator
LinearCombination3::Derivatives& LinearCombination3::Derivatives::operator =(const LinearCombination3::Derivatives& rhs)
{
    if (this != &rhs)
    {
        ColumnMatrix<DCoordinate3>::operator =(rhs);
    }
    return *this;
}

// set every derivative to null vector
GLvoid LinearCombination3::Derivatives::LoadNullVectors()
{
    for (GLuint i = 0; i < _data.size(); ++i)
    {
        for (GLuint j = 0; j < 3; ++j)
            _data[i][0][j] = 0.0;
    }
}

// special constructor
LinearCombination3::LinearCombination3(GLdouble u_min, GLdouble u_max, GLuint data_count, GLenum data_usage_flag):
        _vbo_data(0),
        _data_usage_flag(data_usage_flag),
        _u_min(u_min), _u_max(u_max)
{
    _data.ResizeRows(data_count);
}

// copy constructor
LinearCombination3::LinearCombination3(const LinearCombination3 &lc):
        _vbo_data(0),
        _data_usage_flag(lc._data_usage_flag),
        _u_min(lc._u_min), _u_max(lc._u_max),
        _data(lc._data)
{
    if (lc._vbo_data)
        UpdateVertexBufferObjectsOfData(_data_usage_flag);
}

// assignment operator
LinearCombination3& LinearCombination3::operator =(const LinearCombination3& rhs)
{
    if (this != &rhs)
    {
        DeleteVertexBufferObjectsOfData();

        _data_usage_flag = rhs._data_usage_flag;
        _u_min = rhs._u_min;
        _u_max = rhs._u_max;
        _data = rhs._data;

        if (rhs._vbo_data)
            UpdateVertexBufferObjectsOfData(_data_usage_flag);
    }

    return *this;
}

// vbo handling methods
GLvoid LinearCombination3::DeleteVertexBufferObjectsOfData()
{
    if (_vbo_data)
    {
        glDeleteBuffers(1, &_vbo_data);
        _vbo_data = 0;
    }
}

GLboolean LinearCombination3::RenderData(GLenum render_mode) const
{
    if (!_vbo_data)
        return GL_FALSE;

    if (render_mode != GL_LINE_STRIP && render_mode != GL_LINE_LOOP && render_mode != GL_POINTS)
        return GL_FALSE;

    glEnableClientState(GL_VERTEX_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo_data);
    glVertexPointer(3, GL_FLOAT, 0, (const GLvoid*)0);
    glDrawArrays(render_mode, 0, _data.GetRowCount());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisableClientState(GL_VERTEX_ARRAY);

    return GL_TRUE;
}


GLboolean LinearCombination3::UpdateVertexBufferObjectsOfData(GLenum usage_flag)
{
    GLuint data_count = _data.GetRowCount();
    if (!data_count)
        return GL_FALSE;

    if (usage_flag != GL_STREAM_DRAW  && usage_flag != GL_STREAM_READ  && usage_flag != GL_STREAM_COPY
     && usage_flag != GL_DYNAMIC_DRAW && usage_flag != GL_DYNAMIC_READ && usage_flag != GL_DYNAMIC_COPY
     && usage_flag != GL_STATIC_DRAW  && usage_flag != GL_STATIC_READ  && usage_flag != GL_STATIC_COPY)
        return GL_FALSE;

    _data_usage_flag = usage_flag;

    DeleteVertexBufferObjectsOfData();

    glGenBuffers(1, &_vbo_data);
    if (!_vbo_data)
        return GL_FALSE;

    glBindBuffer(GL_ARRAY_BUFFER, _vbo_data);
    glBufferData(GL_ARRAY_BUFFER, data_count * 3 * sizeof(GLfloat), 0, _data_usage_flag);

    GLfloat *coordinate = (GLfloat*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    if (!coordinate)
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        DeleteVertexBufferObjectsOfData();
        return GL_FALSE;
    }

    for (GLuint i = 0; i < data_count; ++i)
    {
        for (GLuint j = 0; j < 3; ++j)
        {
            *coordinate = (GLfloat)_data[i][j];
            ++coordinate;
        }
    }

    if (!glUnmapBuffer(GL_ARRAY_BUFFER))
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        DeleteVertexBufferObjectsOfData();
        return GL_FALSE;
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return GL_TRUE;
}

// get data by value
DCoordinate3 LinearCombination3::operator [](GLuint index) const
{
    return _data[index];
}

// get data by reference
DCoordinate3& LinearCombination3::operator [](GLuint index)
{
    return _data[index];
}



// assure interpolation
GLboolean LinearCombination3::UpdateDataForInterpolation(const ColumnMatrix<GLdouble>& knot_vector, const ColumnMatrix<DCoordinate3>& data_points_to_interpolate)
{
    GLuint data_count = _data.GetRowCount();

    if (data_count != knot_vector.GetRowCount() ||
        data_count != data_points_to_interpolate.GetRowCount())
        return GL_FALSE;

    RealSquareMatrix collocation_matrix(data_count);

    RowMatrix<GLdouble> current_blending_function_values(data_count);
    for (GLuint r = 0; r < knot_vector.GetRowCount(); ++r)
    {
        if (!BlendingFunctionValues(knot_vector(r), current_blending_function_values))
            return GL_FALSE;
        else
            collocation_matrix.SetRow(r, current_blending_function_values);
    }

    return collocation_matrix.SolveLinearSystem(data_points_to_interpolate, _data);
}


// set/get definition domain
GLvoid LinearCombination3::SetDefinitionDomain(GLdouble u_min, GLdouble u_max)
{
    // done
    _u_min = u_min;
    _u_max = u_max;
}

GLvoid LinearCombination3::GetDefinitionDomain(GLdouble& u_min, GLdouble& u_max) const
{
    // done
    u_min = _u_min;
    u_max = _u_max;
}

// generate image/arc
GenericCurve3* LinearCombination3::GenerateImage(GLuint max_order_of_derivatives, GLuint div_point_count, GLenum usage_flag) const
{
    // done
    if (div_point_count < 2)
    {
        return 0;
    }

    GenericCurve3* result = 0;
    result = new GenericCurve3(max_order_of_derivatives, div_point_count, usage_flag);

    if (!result)
    {
        return 0;
    }

    GLdouble step = (_u_max - _u_min)/(div_point_count - 1);
    Derivatives d;
    for (GLuint i = 0; i < div_point_count; ++i)
    {
        GLdouble u = min(_u_min + i*step, _u_max);
        if (!CalculateDerivatives(max_order_of_derivatives, u, d))
        {
            delete result;
            return 0;
        }
        result->_derivative.SetColumn(i,d);
    }
    return result;

}

GLdouble LinearCombination3::Curvature(GLuint n)
{
    GLdouble step = (_u_max-_u_min)/(2*n);
    GLdouble even = 0.0, odd=0.0;
    GLdouble curvature = 0.0;
    Derivatives d;
    for(GLuint j = 1; j<n; j++)
    {
        GLdouble u = _u_min + 2 * j * step;
        CalculateDerivatives(2,u,d);
        //cout << d[1].length() << "    " << pow(d[1].length(),3) << endl;
        even+=(d[1]^d[2]).length()/pow(d[1].length(),3);
        //even+=(d[1]^d[2]).length()/d[1].length();
        even+= d[1].length();
    }
    for(GLuint j = 1; j<=n; j++)
    {
        GLdouble u = _u_min + 2 * j * step;
        CalculateDerivatives(2,u,d);
        //cout << d[1].length() << "    " << pow(d[1].length(),3) << endl;
        odd += (d[1]^d[2]).length()/pow(d[1].length(),3);
        //odd += (d[1]^d[2]).length()/d[1].length();
        odd += d[1].length();
    }
    curvature = 2*even + 4*odd;
    CalculateDerivatives(2,_u_min,d);
    curvature += (d[1]^d[2]).length()/pow(d[1].length(),3);
    //curvature += (d[1]^d[2]).length()/d[1].length();
    curvature += d[1].length();
    CalculateDerivatives(2,_u_max,d);
    curvature += (d[1]^d[2]).length()/pow(d[1].length(),3);
    //curvature += (d[1]^d[2]).length()/d[1].length();
    curvature += d[1].length();
    curvature *= step;
    curvature /= 3.0;

    return curvature;
}

GLdouble LinearCombination3::Length(GLuint n)
{
    GLdouble step = (_u_max-_u_min)/(2*n);
    GLdouble even = 0.0, odd=0.0;
    GLdouble curvature = 0.0;
    Derivatives d;
    for(GLuint j = 1; j<n; j++)
    {
        GLdouble u = _u_min + 2 * j * step;
        CalculateDerivatives(1,u,d);
        even+= d[1].length();
    }
    for(GLuint j = 1; j<=n; j++)
    {
        GLdouble u = _u_min + 2 * j * step;
        CalculateDerivatives(1,u,d);
        odd += d[1].length();
    }

    curvature = 2*even + 4*odd;
    CalculateDerivatives(2,_u_min,d);
    curvature += d[1].length();
    CalculateDerivatives(2,_u_max,d);
    curvature += d[1].length();
    curvature *= step;
    curvature /= 3.0;

    return curvature;
}

GLdouble LinearCombination3::Fitness(GLuint n, RowMatrix<GLdouble>& eProportion)
{
    GLdouble step = (_u_max-_u_min)/(2*n);
    GLdouble even = 0.0, odd=0.0;
    GLdouble fitness = 0.0;
    GLdouble w_e = eProportion[0];
    GLdouble w_c = eProportion[1];
    Derivatives d;
    for(GLuint j = 1; j<n; j++)
    {
        GLdouble u = _u_min + 2 * j * step;
        CalculateDerivatives(2,u,d);
        even += w_c * (d[1]^d[2]).length()/pow(d[1].length(),3);
        even += w_e * d[1].length();
    }
    for(GLuint j = 1; j<=n; j++)
    {
        GLdouble u = _u_min + 2 * j * step;
        CalculateDerivatives(2,u,d);
        odd += w_c * (d[1]^d[2]).length()/pow(d[1].length(),3);
        odd += w_e * d[1].length();
    }
    fitness = 2*even + 4*odd;
    CalculateDerivatives(2,_u_min,d);
    fitness += w_c * (d[1]^d[2]).length()/pow(d[1].length(),3);
    fitness += w_e * d[1].length();
    CalculateDerivatives(2,_u_max,d);
    fitness += w_c * (d[1]^d[2]).length()/pow(d[1].length(),3);
    fitness += w_e * d[1].length();
    fitness *= step;
    fitness /= 3.0;

    return fitness;
}

// destructor
LinearCombination3::~LinearCombination3()
{
    DeleteVertexBufferObjectsOfData();
}

