package expo.modules.spatiallayer

import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import com.google.android.gms.maps.model.Tile
import com.google.android.gms.maps.model.TileProvider
import java.io.ByteArrayOutputStream

/**
 * TileProvider that renders spatial points as map tiles.
 * Points are rendered directly into the map's tile system, ensuring perfect synchronization.
 */
class SpatialTileProvider(
    private var module: ExpoSpatialLayerModule? = null,
    private val tileSize: Int = 512, // Higher resolution for retina displays
    private val pointRadius: Float = 6f,
    private val pointColor: Int = Color.CYAN
) : TileProvider {
    
    private val paint = Paint().apply {
        color = pointColor
        isAntiAlias = true
        style = Paint.Style.FILL
    }
    
    private var pointStyles: Map<Int, Int> = emptyMap()
    
    override fun getTile(x: Int, y: Int, zoom: Int): Tile? {
        // Get points for this tile from C++ layer
        // Format: [x, y, type, x, y, type, ...]
        val points = getPointsForTile(x, y, zoom)
        
        if (points == null || points.isEmpty()) {
            return TileProvider.NO_TILE
        }
        
        // Create bitmap and canvas
        val bitmap = Bitmap.createBitmap(tileSize, tileSize, Bitmap.Config.ARGB_8888)
        val canvas = Canvas(bitmap)
        
        // Draw each point
        for (i in points.indices step 3) {
            val px = points[i] * tileSize
            val py = points[i + 1] * tileSize
            val type = points[i + 2].toInt()
            
            // Set color based on type or default
            paint.color = pointStyles[type] ?: pointColor
            
            canvas.drawCircle(px, py, pointRadius, paint)
        }
        
        // Convert to PNG bytes
        val stream = ByteArrayOutputStream()
        bitmap.compress(Bitmap.CompressFormat.PNG, 100, stream)
        val data = stream.toByteArray()
        bitmap.recycle()
        
        return Tile(tileSize, tileSize, data)
    }
    
    /**
     * Get points within a specific tile.
     * Returns array of [x, y, x, y, ...] normalized to [0, 1] within the tile.
     */
    private fun getPointsForTile(tileX: Int, tileY: Int, zoom: Int): FloatArray? {
        return module?.getPointsForTile(tileX, tileY, zoom)
    }
    
    fun setModule(module: ExpoSpatialLayerModule) {
        this.module = module
    }

    fun setColor(color: Int) {
        paint.color = color
    }
    
    fun setRadius(radius: Float) {
        // Store for next render - can't change paint during getTile calls
    }

    fun setPointStyles(styles: Map<Int, Int>) {
        this.pointStyles = styles
    }
}
