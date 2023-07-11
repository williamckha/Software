import pyqtgraph as pg


class GLLayer(pg.GraphicsObject):
    """Represents a layer in the 3D visualization. A layer manages and returns GLGraphicsItem 
    graphics to be displayed in the 3D scene.
    """

    def __init__(self):
        """Initialize the GLLayer"""
        pg.GraphicsObject.__init__(self)

        # Options for the layer, used to configure the legend
        self.opts = {
            "pxMode": True,
            "useCache": True,
            "antialias": True,
            "name": None,
            "symbol": "o",
            "size": 7,
            "pen": pg.mkPen("w"),
            "brush": pg.mkBrush("w"),
            "tip": "x: {x:.3g}\ny: {y:.3g}\ndata={data}".format,
        }

    def updateGraphics(self):
        """Update the GLGraphicsItems in this layer

        :returns: tuple (added_graphics, removed_graphics)
            - added_graphics - List of the added GLGraphicsItems
            - removed_graphics - List of the removed GLGraphicsItems
        
        """
        raise NotImplementedError("Abstract class method called: updateGraphics")

    def setupGraphicsList(self, graphics_list, num_graphics, graphic_init_func):
        """Add or remove graphics from the given graphics list so that its
        length is equal to the specified number of graphics.

        :param graphics_list: The list of graphics to setup
        :param num_graphics: The number of graphics that should be in the list
        :param graphic_init_func: A function that returns a graphic to add to the list

        :returns: tuple (added_graphics, removed_graphics)
            - added_graphics - List of the added graphics
            - removed_graphics - List of the removed graphics

        """
        added_graphics = []
        removed_graphics = []

        if len(graphics_list) < num_graphics:

            num_graphics_to_add = num_graphics - len(graphics_list)

            for i in range(num_graphics_to_add):
                graphic = graphic_init_func()
                graphics_list.append(graphic)
                added_graphics.append(graphic)

        elif len(graphics_list) > num_graphics:

            num_graphics_to_remove = len(graphics_list) - num_graphics

            for i in range(num_graphics_to_remove):
                removed_graphics.append(graphics_list.pop())

        return added_graphics, removed_graphics

    def clearGraphicsList(self, graphics_list):
        """Clear the given graphics list
        
        :returns: List of the removed graphics
        
        """
        removed_graphics = []
        for index in range(len(graphics_list)):
            removed_graphics.append(graphics_list.pop())

        return removed_graphics

    def clearGraphicsDict(self, graphics_dict):
        """Clear the given graphics dictionary
        
        :returns: List of the removed graphics

        """
        removed_graphics = []
        for key in list(graphics_dict):
            removed_graphics.append(graphics_dict.pop(key))

        return removed_graphics